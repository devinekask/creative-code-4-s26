/*
    Bidirectional BLE Demo: Joystick + RGB LED
    
    Based on Neil Kolban example for IDF
    Ported to Arduino ESP32 by Evandro Copercini

   Create a BLE server for bidirectional communication:
   - Joystick values are sent via NOTIFY characteristics
   - RGB LED values are received via WRITE characteristics

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create BLE Characteristics on the Service
   4. Create BLE Descriptors on the characteristics
   5. Start the service.
   6. Start advertising.
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLE2901.h>

// Joystick pins
const int xPin = 15;
const int yPin = 4;
const int buttonPin = 16;

// RGB LED pins
const int redPin = 18;
const int greenPin = 19;
const int bluePin = 21;

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristicX = NULL;
BLECharacteristic *pCharacteristicY = NULL;
BLECharacteristic *pCharacteristicButton = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;

int prevX = 0;
int prevY = 0;
int prevButton = LOW;

// Timing for non-blocking joystick updates
unsigned long previousMillis = 0;
const unsigned long interval = 50;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID               "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
// Joystick characteristics (NOTIFY) - Arduino sends to Web
#define CHARACTERISTIC_X_UUID      "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_Y_UUID      "6e400003-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_BUTTON_UUID "6e400004-b5a3-f393-e0a9-e50e24dcca9e"
// RGB LED characteristics (WRITE) - Web sends to Arduino
#define CHARACTERISTIC_R_UUID      "6e400005-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_G_UUID      "6e400006-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_B_UUID      "6e400007-b5a3-f393-e0a9-e50e24dcca9e"

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    Serial.println("Device connected");
  };

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
    Serial.println("Device disconnected");
  }
};

class RedCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String rxValue = pCharacteristic->getValue();
    if (rxValue.length() > 0) {
      uint8_t value = (uint8_t)rxValue[0];
      analogWrite(redPin, value);
      Serial.print("Red: ");
      Serial.println(value);
    }
  }
};

class GreenCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String rxValue = pCharacteristic->getValue();
    if (rxValue.length() > 0) {
      uint8_t value = (uint8_t)rxValue[0];
      analogWrite(greenPin, value);
      Serial.print("Green: ");
      Serial.println(value);
    }
  }
};

class BlueCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String rxValue = pCharacteristic->getValue();
    if (rxValue.length() > 0) {
      uint8_t value = (uint8_t)rxValue[0];
      analogWrite(bluePin, value);
      Serial.print("Blue: ");
      Serial.println(value);
    }
  }
};

void setup() {
  // Joystick button input
  pinMode(buttonPin, INPUT_PULLUP);
  
  // RGB LED outputs
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  
  Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init("ESP32");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(BLEUUID(SERVICE_UUID), 32);

  // Create BLE Characteristics for Joystick X, Y, and Button (NOTIFY)
  pCharacteristicX = pService->createCharacteristic(
    CHARACTERISTIC_X_UUID,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pCharacteristicX->addDescriptor(new BLE2902());

  pCharacteristicY = pService->createCharacteristic(
    CHARACTERISTIC_Y_UUID,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pCharacteristicY->addDescriptor(new BLE2902());

  pCharacteristicButton = pService->createCharacteristic(
    CHARACTERISTIC_BUTTON_UUID,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pCharacteristicButton->addDescriptor(new BLE2902());

  // Create BLE Characteristics for RGB LED (WRITE)
  BLECharacteristic *pCharacteristicR = pService->createCharacteristic(
    CHARACTERISTIC_R_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
  );
  pCharacteristicR->setCallbacks(new RedCallback());

  BLECharacteristic *pCharacteristicG = pService->createCharacteristic(
    CHARACTERISTIC_G_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
  );
  pCharacteristicG->setCallbacks(new GreenCallback());

  BLECharacteristic *pCharacteristicB = pService->createCharacteristic(
    CHARACTERISTIC_B_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
  );
  pCharacteristicB->setCallbacks(new BlueCallback());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection...");
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Send joystick data at interval
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    int xValue = analogRead(xPin);
    int yValue = analogRead(yPin);
    int buttonValue = digitalRead(buttonPin);
    
    // notify changed values
    if (deviceConnected) {
      if (xValue != prevX) {
        pCharacteristicX->setValue((uint8_t *)&xValue, 4);
        pCharacteristicX->notify();
      }
      if (yValue != prevY) {
        pCharacteristicY->setValue((uint8_t *)&yValue, 4);
        pCharacteristicY->notify();
      }
      if (buttonValue != prevButton) {
        pCharacteristicButton->setValue((uint8_t *)&buttonValue, 4);
        pCharacteristicButton->notify();
      }
    }
    
    prevX = xValue;
    prevY = yValue;
    prevButton = buttonValue;
  }
  
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);                   // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising();  // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}

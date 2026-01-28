#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

// Make sure to set a unique name for your device here:
#define DEVICE_NAME "BLE Potmeter RGB"

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID               "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_UUID_R      "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_UUID_G      "6e400003-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_UUID_B      "6e400004-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_X_UUID      "6e400005-b5a3-f393-e0a9-e50e24dcca9e"

const int xPin = 15;

// pins for the LEDs:
const int redPin = 4;
const int greenPin = 5;
const int bluePin = 6;

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristicX = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;

int prevX = 0;

unsigned long previousMillis = 0;
const unsigned long interval = 50;

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
  Serial.begin(115200);

  // Set up LED pins
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  // Create the BLE Device
  BLEDevice::init(DEVICE_NAME);

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic *pRCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_R, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  pRCharacteristic->setCallbacks(new RedCallback());

  BLECharacteristic *pGCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_G, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  pGCharacteristic->setCallbacks(new GreenCallback());

  BLECharacteristic *pBCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_B, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  pBCharacteristic->setCallbacks(new BlueCallback());

  pCharacteristicX = pService->createCharacteristic(
    CHARACTERISTIC_X_UUID,
    BLECharacteristic::PROPERTY_NOTIFY
  );

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);  // This is crucial for Web Bluetooth to find the device!
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // helps with iPhone connection issues
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
  unsigned long currentMillis = millis();
  
  // notify changed values (non-blocking)
  if (deviceConnected && (currentMillis - previousMillis >= interval)) {
    previousMillis = currentMillis;
    int xValue = analogRead(xPin);
    if (xValue != prevX) {
      pCharacteristicX->setValue((uint8_t *)&xValue, 4);
      pCharacteristicX->notify();
      prevX = xValue;
    }
  }
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);                   // give the bluetooth stack the chance to get things ready
    BLEDevice::startAdvertising();  // restart advertising
    Serial.println("Started advertising again...");
    oldDeviceConnected = false;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = true;
  }
}
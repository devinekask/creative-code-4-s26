/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updated by chegewara

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 4fafc201-1fb5-459e-8fcc-c5c9c331914b
   And has a characteristic of: beb5483e-36e1-4688-b7f5-ea07361b26a8

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   A connect handler associated with the server starts a background task that performs notification
   every couple of seconds.
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLE2901.h>

const int xPin = 15;
const int yPin = 4;
const int buttonPin = 16;

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

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID               "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_X_UUID      "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_Y_UUID      "6e400003-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_BUTTON_UUID "6e400004-b5a3-f393-e0a9-e50e24dcca9e"

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
  }
};

void setup() {
  pinMode(buttonPin, INPUT_PULLUP);
  Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init("ESP32");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create BLE Characteristics for X, Y, and Button
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

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
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
    delay(50);
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
  prevX = xValue;
  prevY = yValue;
  prevButton = buttonValue;
}

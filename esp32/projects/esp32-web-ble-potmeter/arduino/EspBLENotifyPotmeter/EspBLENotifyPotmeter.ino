#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

// Make sure to set a unique name for your device here:
#define DEVICE_NAME "BLE Potmeter"

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID               "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_X_UUID      "6e400005-b5a3-f393-e0a9-e50e24dcca9e"

const int xPin = 15;

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristicX = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;

int prevX = 0;

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

void setup() {
  Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init(DEVICE_NAME);

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create BLE Characteristic for sensor
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
  int xValue = analogRead(xPin);
  // notify changed values
  if (deviceConnected) {
    if (xValue != prevX) {
      pCharacteristicX->setValue((uint8_t *)&xValue, 4);
      pCharacteristicX->notify();
    }
    delay(50);
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
  prevX = xValue;
}

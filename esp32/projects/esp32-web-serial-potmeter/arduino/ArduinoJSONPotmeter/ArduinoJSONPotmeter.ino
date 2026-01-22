#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

const int sensorPin = 7;

void setup() {
  Serial.begin(115200);
  while (!Serial) continue;
}

void loop() {
  int value = analogRead(sensorPin);

  DynamicJsonDocument doc(1024);

  doc["sensor"] = "potmeter";
  doc["data"][0] = value;

  serializeJson(doc, Serial);
  Serial.println();
  delay(100);
}
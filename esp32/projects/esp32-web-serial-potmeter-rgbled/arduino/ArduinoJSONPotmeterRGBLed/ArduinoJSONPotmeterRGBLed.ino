#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

// sensor pins
const int sensorPin = 7;

// RGB LED pins
const int redPin = 4;
const int greenPin = 5;
const int bluePin = 6;

// Timing for non-blocking sensor data send
unsigned long previousMillis = 0;
const unsigned long interval = 100;

void setup() {
  // RGB LED outputs
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  
  Serial.begin(115200);
  while (!Serial) continue;
}

void loop() {
  // Handle incoming RGB LED commands
  if (Serial.available() > 0) {
    String s = Serial.readStringUntil('\n');
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, s);
    if (!error) {
      int red = doc["r"];
      int green = doc["g"];
      int blue = doc["b"];

      red = constrain(red, 0, 255);
      green = constrain(green, 0, 255);
      blue = constrain(blue, 0, 255);

      analogWrite(redPin, red);
      analogWrite(greenPin, green);
      analogWrite(bluePin, blue);
    }
  }

  // Send potmeter data at interval
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    int sensorValue = analogRead(sensorPin);

    DynamicJsonDocument doc(1024);

    doc["sensor"] = "potmeter";
    doc["data"][0] = sensorValue;

    serializeJson(doc, Serial);
    Serial.println();
  }
}
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// ====== Wi-Fi Credentials ======
const char* ssid = "V50LG";
const char* password = "Myumobile";

// ====== Google Script Web App URL ======
const char* scriptURL = "https://script.google.com/macros/s/AKfycbyUGTvqEiGD7nIJ5f7wJFgE23zdM9nyYVcYAsMkJOVgwRXTnGEzQgyqUVgEZCQABNxR/exec";

// ====== Ultrasonic Pins ======
#define TRIG_PIN 13
#define ECHO_PIN 12

// ====== BME280 Setup ======
Adafruit_BME280 bme;
#define SEALEVELPRESSURE_HPA (1013.25)

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.println(WiFi.localIP());

  // Initialize BME280
  if (!bme.begin(0x76)) {
    Serial.println("Could not find BME280 sensor! Check wiring or address.");
    while (1);
  } else {
    Serial.println("BME280 sensor detected!");
  }
}

void loop() {
  float distance = getDistance();
  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F;
  float pressure_hpa = bme.readPressure() / 100.0F;
  float pressure_atm = pressure_hpa / 1013.25;

  Serial.println("=== Sensor Readings ===");
  Serial.printf("Distance: %.2f cm\n", distance);
  Serial.printf("Temperature: %.2f °C\n", temperature);
  Serial.printf("Humidity: %.2f %%\n", humidity);
  Serial.printf("Pressure: %.5f atm\n\n", pressure_atm);

  sendToGoogleSheet(distance, temperature, humidity, pressure_atm);

  delay(10000); // Send every 10 seconds
}

float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = duration * 0.034 / 2;
  return distance;
}

void sendToGoogleSheet(float distance, float temperature, float humidity, float pressure) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(scriptURL);
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<200> doc;
    doc["distance"] = distance;
    doc["temperature"] = temperature;
    doc["humidity"] = humidity;
    doc["pressure"] = pressure;

    String jsonString;
    serializeJson(doc, jsonString);

    int httpResponseCode = http.POST(jsonString);
    if (httpResponseCode > 0) {
      Serial.println("Data sent successfully: " + String(httpResponseCode));
      Serial.println(http.getString());
    } else {
      Serial.println("Error sending data: " + String(httpResponseCode));
    }
    http.end();
  } else {
    Serial.println("WiFi disconnected!");
  }
}

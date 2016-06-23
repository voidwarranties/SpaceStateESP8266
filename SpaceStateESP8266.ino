/*
  Space State ESP8266

  This sketch uses an ESP8266-01 chip to monitor the opening and closing of
  the Voidwarranties hackerspace.
  It monitors a digital pin (GPIO0) and it periodically updates
  the spaceapi accordingly.
  It also monitors the temperature and humidity in the hackerspace through
  the use of a DHT22 sensor connected to GPIO2.

  The current implementation uses a basic ESP8266-01 board loaded with the
  Arduino core for the ESP8266 WiFi chip which can be found here:
  https://github.com/esp8266/Arduino

  The circuit:
  * Pin GPIO0 (D1): Internal pullup enabled. Connected to GROUND with a switch (the
    space state lever in the space). When the circuit is closed, the pin is
    pulled low and Voidwarranties is considered open. When the circuit is
    opened, the pin is pulled high by the internal pullup and
    Voidwarranties is considered closed.
  * Pin GPIO2 (D2): DHT22 temperature and humidity sensor.

  Created 22nd of June 2016
  By Michael Smith
  http://we.voidwarranties.be/
  This software is licensed under MIT.

*/

#include <Arduino.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// Flags
boolean serialDebug = false;

// Variables
char ssid[] = "WIFI_SSID";           // SSID of WiFi network to connect to
char password[] = "";                         // Passphrase of WiFi network

// URL of the space API endpoint:
char space_api_url[] = "SPACE_STATE_API_ENDPOINT";
// URL of the Thingspeak API endpoint:
char thingspeak_api_url[] = "http://api.thingspeak.com/update";
// Thingspeak channel write API key:
const char* apiKey = "THINGSPEAK_API_KEY";

int switchPin = D1;                     // Digital pin used to form circuit
                                        // to ground
int DHTpin = D2;                        // Digital pin to which the DHT22
                                        // sensor is connected
int updateDelay = 60000;                // Interval between updates on space
                                        // state (60000 = 60s)

// Objects
DHT dht(DHTpin, DHT22);

void setup() {
  if (serialDebug) Serial.begin(9600);
  pinMode(switchPin, INPUT_PULLUP);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  WiFi.begin(ssid, password);
  dht.begin();
}

void loop() {
  Serial.println("Ready.");
  // Read temperature and humidity from DHT22 sensor
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  // Read humidity
  float h = dht.readHumidity();
  // Read temperature
  float t = dht.readTemperature();

  // Wait for WiFi connection:
  if(WiFi.status() == WL_CONNECTED) {
    // Set up HTTP request:
    HTTPClient http;
    http.begin(space_api_url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    // Check if space is closed (circuit open):
    if (digitalRead(switchPin) == HIGH) {
      if (serialDebug) Serial.println("Space closed");
      http.POST("state=closed");
    // Check if space is open (circuit closed):
    } else {
      if (serialDebug) Serial.println("Space open");
      http.POST("state=open");
    }
    http.end();

    // Report temperature and humidity to thingspeak
    if (isnan(h) || isnan(t)) {
      if (serialDebug) Serial.println("Failed to read from DHT sensor!");
    } else {
      http.begin(thingspeak_api_url);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      http.addHeader("X-THINGSPEAKAPIKEY", apiKey);
      String postStr = apiKey;
             postStr +="&field1=";
             postStr += String(t);
             postStr +="&field2=";
             postStr += String(h);
      http.POST(postStr);
      http.end();

      if (serialDebug) {
        Serial.print("Humidity: ");
        Serial.print(h);
        Serial.print(" %\t");
        Serial.print("Temperature: ");
        Serial.print(t);
        Serial.println(" *C ");
      }
    }
  }

  // Wait a while before the next update
  delay(updateDelay);
}

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
    Pin GPIO0 (D1): Internal pullup enabled. Connected to GROUND with a switch
    (the space state lever in the space). When the circuit is closed, the pin is
    pulled low and Voidwarranties is considered open. When the circuit is
    opened, the pin is pulled high by the internal pullup and
    Voidwarranties is considered closed.
    Pin GPIO2 (D2): DHT22 temperature and humidity sensor.

  Created 22nd of June 2016
  Updated 4th of December 2019 to push temperature and humidity to MQTT
  Updated 11th of March 2020 to use mDNS to reach the MQTT server, publish the
  space state to the MQTT server, separate configuration file support and a
  general code cleanup.
  By Michael Smith
  http://we.voidwarranties.be/
  This software is licensed under MIT.

*/

#include <Arduino.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "config.h"

// WiFi config
char ssid[] = WLANSSID;
char password[] = WLANPWD;

// MQTT config
char mqtt_server[] = MQTT_SERVER;
int mqtt_port = MQTT_PORT;
char mqtt_user[] = MQTT_USER;
char mqtt_password[] = MQTT_PASSWORD;

// Space State API config
char space_api_url[] = SPACE_API_URL;

// Hardware config
int switchPin = STATE_SWITCH_PIN;
int DHTpin = DHT_PIN;
int updateDelay = UPDATE_DELAY;

int spaceState = 0;

// Objects
DHT dht(DHTpin, DHT22);

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, mqtt_server, mqtt_port, mqtt_user,
                          mqtt_password);
Adafruit_MQTT_Publish dhtFeed = Adafruit_MQTT_Publish(&mqtt, MQTT_TOPIC);

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print(F("Connecting to MQTT... "));

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) {
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println(F("Retrying MQTT connection in 5 seconds..."));
    mqtt.disconnect();
    delay(5000);
    retries--;
    if (retries == 0) {
      return;
    }
  }
  Serial.println(F("MQTT Connected!"));
}

void setup() {
  Serial.begin(9600);
  pinMode(switchPin, INPUT_PULLUP);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println("");
  Serial.print(F("Connected to "));
  Serial.println(ssid);
  Serial.print(F("IP address: "));
  Serial.println(WiFi.localIP());

  // Set up mDNS responder
  if (!MDNS.begin(F("spacestate"))) {
    Serial.println(F("Error setting up MDNS responder!"));
    while (1) {
      delay(1000);
    }
  }
  Serial.println(F("mDNS responder started"));

  dht.begin();
}

void loop() {
  MDNS.update();

  // Read temperature and humidity from DHT22 sensor
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)

  // Read humidity
  float h = dht.readHumidity();
  // Read temperature
  float t = dht.readTemperature();

  // Wait for WiFi connection:
  if (WiFi.status() == WL_CONNECTED) {
    // Set up HTTP request:
    HTTPClient http;
    http.begin(space_api_url);
    http.addHeader(F("Content-Type"), F("application/x-www-form-urlencoded"));

    // Check if space is closed (circuit open):
    if (digitalRead(switchPin) == HIGH) {
      spaceState = 0;
      Serial.println(F("Space closed"));
      http.POST(F("state=closed"));
      // Check if space is open (circuit closed):
    } else {
      spaceState = 1;
      Serial.println(F("Space open"));
      http.POST(F("state=open"));
    }
    http.end();

    // Report temperature and humidity to MQTT
    if (isnan(h) || isnan(t)) {
      Serial.println(F("Failed to read from DHT sensor!"));
    } else {
      // Push to MQTT
      MQTT_connect();
      char mqtt_buffer[200];
      snprintf(mqtt_buffer,
               200,
               "{\"temperature\": %.2f,\"humidity\": %.2f,\"state\": %d}",
               t,
               h,
               spaceState);

      Serial.println(F("\nSending iaq data:"));
      Serial.println(mqtt_buffer);
      Serial.print(F("..."));
      if (! dhtFeed.publish(mqtt_buffer)) {
        Serial.println(F("Failed"));
      } else {
        Serial.println(F("OK!"));
      }

      Serial.print(F("Humidity: "));
      Serial.print(h);
      Serial.print(F(" %\t"));
      Serial.print(F("Temperature: "));
      Serial.print(t);
      Serial.println(F(" *C "));
    }
  }

  delay(updateDelay);
}

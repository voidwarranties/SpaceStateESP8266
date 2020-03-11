// WiFi config
#define WLANSSID ""       // SSID of WiFi network to connect to
#define WLANPWD ""        // Passphrase of WiFi network

// MQTT config
#define MQTT_SERVER ""    // IP or mDNS name of the MQTT server to push readings
                          // to
#define MQTT_PORT 1883
#define MQTT_USER ""                  // MQTT authentication username
#define MQTT_PASSWORD ""              // MQTT authentication password
#define MQTT_TOPIC "environment/dht"  // MQTT topic

// Space State API config
#define SPACE_API_URL ""        // URL of the Space State API endpoint
#define UPDATE_DELAY 60000      // Interval between updates on space
                                // state (60000 = 60s)

// Hardware config
#define STATE_SWITCH_PIN D1     // Digital pin used to form circuit
                                // to ground
#define DHT_PIN D2              // Digital pin to which the DHT22 sensor is
                                // connected

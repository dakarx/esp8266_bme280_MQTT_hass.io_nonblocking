/*
    V0.2.1 - Dave Cochran Feb 5, 2021
    bme/bmp280 sensor output to MQTT

    Rewrite of MQTT_Temp_Hum_Baro_Sensor.ino located at https://gist.github.com/mtl010957/9ee85fb404f65e15c440b08c659c0419
    The connection to the MQTT server flooded the server with dozens of connection per second and created a DoS,
    there is an issue with the client.connected() bool state never being updated creating a reconnect loop.
    Gave up troubleshooting/repair and decided to learn things and rewrite it.

    The orginal sensor logic has been retained, but the connection established amd maintained bit are now using the example code contained the PubSubClient library.

    Sensor is monitored for changes, if a change in value is detected it's sent to the MQTT server, else current values are force sent every 5 mins to keep things alive.
*/

#include <SPI.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define DEBUG   // Uncomment to Print output debug statements to Serial

#define wifi_ssid "SSID"
#define wifi_password "WIFI_PASSWD"
#define mqtt_server "MQTT_SERVER"
#define mqtt_user "MQTT_USER"
#define mqtt_password "MQTT_PASSWD"

// If using multiple sensors change the sensor_number below  ( Topic will read like /sensor/2/humidity/percentRelative )
#define sensor_number "1"
#define humidity_topic "sensor/" sensor_number "/humidity/percentRelative"
#define temperature_c_topic "sensor/" sensor_number "/temperature/degreeCelsius"
#define temperature_f_topic "sensor/" sensor_number "/temperature/degreeFahrenheit"
#define barometer_hpa_topic "sensor/" sensor_number "/barometer/hectoPascal"
#define barometer_inhg_topic "sensor/" sensor_number "/barometer/inchHg"
// Lookup for your altitude and fill in here, units hPa
// Positive for altitude above sea level
#define baro_corr_hpa 34.5879 // = 289m above sea level

void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
}

WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_BME280 bme; // I2C

long lastReconnectAttempt = 0;

boolean reconnect() {
  if (client.connect("espClient", mqtt_user, mqtt_password)) {
    Serial.println("connected");
    //Once connected, publish an announcement...  Uncomment below to test pub/sub topics
    //client.publish("outTopic", "hello world");
    // ... and resubscribe
    //client.subscribe("inTopic");
  }
  return client.connected();
}

void setup(){
  #if defined(DEBUG)
  Serial.begin(115200);
  delay(250); // wait for console opening
  #endif
  
  setup_wifi();
  client.setServer(mqtt_server, 1883);

  // Start sensor
  if (!bme.begin()) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(480);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

bool checkBound(float newValue, float prevValue, float maxDiff) {
  return newValue < prevValue - maxDiff || newValue > prevValue + maxDiff;
}

long lastMsg = 0;
long lastForceMsg = 0;
bool forceMsg = false;
float temp = 0.0;
float hum = 0.0;
float baro = 0.0;
float diff = 1.0;

void loop() {
  if (!client.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
      }
    }
  } else {
    // Client connected
    client.loop();

    long now = millis();
    if (now - lastMsg > 1000) {
      lastMsg = now;

      // MQTT broker could go away and come back at any time
      // so doing a forced publish to make sure something shows up
      // within the first 5 minutes after a reset
      if (now - lastForceMsg > 300000) {
        lastForceMsg = now;
        forceMsg = true;
        Serial.println("Forcing publish every 5 minutes...");
      }

      float newTemp = bme.readTemperature();
      float newHum = bme.readHumidity();
      float newBaro = bme.readPressure() / 100.0F;

      if (checkBound(newTemp, temp, diff) || forceMsg) {
        temp = newTemp;
        float temp_c = temp; // Celsius
        float temp_f = temp * 1.8F + 32.0F; // Fahrenheit
        Serial.print("New temperature:");
        Serial.print(String(temp_c) + " degC   ");
        Serial.println(String(temp_f) + " degF");
        client.publish(temperature_c_topic, String(temp_c).c_str(), true);
        client.publish(temperature_f_topic, String(temp_f).c_str(), true);
      }

      if (checkBound(newHum, hum, diff) || forceMsg) {
        hum = newHum;
        Serial.print("New humidity:");
        Serial.println(String(hum) + " %");
        client.publish(humidity_topic, String(hum).c_str(), true);
      }
      
      if (checkBound(newBaro, baro, diff) || forceMsg) {
        baro = newBaro;
        float baro_hpa = baro + baro_corr_hpa; // hPa corrected to sea level
        float baro_inhg = (baro + baro_corr_hpa) / 33.8639F; // inHg corrected to sea level
        Serial.print("New barometer:");
        Serial.print(String(baro_hpa) + " hPa   ");
        Serial.println(String(baro_inhg) + " inHg");
        client.publish(barometer_hpa_topic, String(baro_hpa).c_str(), true);
        client.publish(barometer_inhg_topic, String(baro_inhg).c_str(), true);
      }
      forceMsg = false;
    }
  }
}

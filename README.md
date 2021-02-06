# esp8266_bme280_MQTT_hass.io_nonblocking
ESP8266 BME/BMP280 to MQTT (for HASS.IO)

    V0.2.1 - Dave Cochran Feb 5, 2021
    bme/bmp280 sensor output to MQTT

    Rewrite of MQTT_Temp_Hum_Baro_Sensor.ino located at https://gist.github.com/mtl010957/9ee85fb404f65e15c440b08c659c0419
    The connection to the MQTT server flooded the server with dozens of connection per second and created a DoS,
    there is an issue with the client.connected() bool state never being updated creating a reconnect loop.
    Gave up troubleshooting/repair and decided to learn things and rewrite it.

    The orginal sensor logic has been retained, but the connection established amd maintained bit are now using the example code contained the PubSubClient library.

    Sensor is monitored for changes, if a change in value is detected it's sent to the MQTT server, else current values are force sent every 5 mins to keep things alive.

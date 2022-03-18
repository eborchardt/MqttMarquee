
# MQTT Marquee

Uses an ESP32 to pull text from an MQTT broker and then scrolls the text across an SSD1306 OLED display.

The code is a bit of a mess, but it works. I'll work on cleaning it up later, but it is based off of the two Adafruit libraries, Adafruit_MQTT_Client and Adafruit_SSD1306. I had to use the second core on the ESP32 in order to get smooth scrolling, but it can also be done with a while loop on a single core. It just doesn't look as pretty. 



## Usage/Examples

First edit esp32_mqtt/arduino_secrets.h with your information.

```
#define SECRET_SSID "<Name of your WiFi SSID>"
#define SECRET_PASS "<WiFi Password>"
#define MQTT_SERVER "<The address or URL of your MQTT broker>"
#define MQTT_USER "<MQTT Username>"
#define MQTT_PASS "<MQTT Password>"
```

Then, define the MQTT topic with:

```
Adafruit_MQTT_Subscribe <unique ID> = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "<your topic>");
```

Then add/change your <unique ID> to the checkSubs() function like:
  
```
void checkSubs()  {
  Adafruit_MQTT_Subscribe *subscription;
    while ((subscription = mqtt.readSubscription(5000))) {
      if (subscription == &<unique ID>) {
        Serial.print(F("Got: "));
        Serial.println((char *)<unique ID>.lastread);
        }
    }
}
```

#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "arduino_secrets.h"

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       SECRET_SSID
#define WLAN_PASS       SECRET_PASS

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      MQTT_SERVER
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    MQTT_USER
#define AIO_KEY         MQTT_PASS

/************************* SSD1306 Setup ************************************/
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

TaskHandle_t Task1;

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiClientSecure for SSL
//WiFiClientSecure client;



// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// Setup a feed called 'photocell' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish photocell = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/photocell");

// Setup a feed called 'onoff' for subscribing to changes.
Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/onoff");

// Setup a feed called 'media_title' for subscribing to changes:
Adafruit_MQTT_Subscribe media_title = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/media_player/googlehome8001/media_title");

/*************************** Sketch Code ************************************/

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).

void MQTT_connect();

int x, minX;
String song_title;
char title[100];

void setup() {

  
  Serial.begin(9600);
  delay(10);
  
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Clear the buffer
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);
  x = display.width();
  
  Serial.println(); Serial.println();
  Serial.println(F("MQTT & SSD1306 Test"));

  // Connect to WiFi access point.
  Serial.println(); 
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());

  // Setup MQTT subscription for onoff feed.
  mqtt.subscribe(&onoffbutton);
  mqtt.subscribe(&media_title);

  xTaskCreatePinnedToCore(
             Task1code, /* Task function. */
             "Task1",   /* name of task. */
             5000,     /* Stack size of task */
             NULL,      /* parameter of the task */
             1,         /* priority of the task */
             &Task1,    /* Task handle to keep track of created task */
             1);        /* pin task to core 1 */
}

//uint32_t x=0;

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  if(! mqtt.connected()) {
    MQTT_connect();
  }

  //This are the main functions
  checkSubs();


//  // Now we can publish stuff!
//  Serial.print(F("\nSending photocell val "));
//  Serial.print(x);
//  Serial.print("...");
//  if (! photocell.publish(x++)) {
//    Serial.println(F("Failed"));
//  } else {
//    Serial.println(F("OK!"));
//  }

  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }

}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}


void scrollText(char message[]) {
  Serial.print("Got: ");
  Serial.println(message);
  minX = -12 * strlen(message); //12 = 6 pixels/character * text size 2
  Serial.print("strlen(message): ");
  Serial.println(strlen(message));  
  while (x >= minX) {
//    Serial.print("minX: ");
//    Serial.println(minX);
//    Serial.print("x: ");
//    Serial.println(x);
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(x,7);
    display.print(message);
    display.display();
    x = x - 2; // scroll speed
  }

  if (x < minX) x = display.width();
}

void testscrolltext(String text) {
  display.clearDisplay();
  display.setTextSize(2); // Draw text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 0);
  display.println(text);
  display.display();
  display.startscrollleft(0x00, 0x0F);
}

void checkSubs()  {
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    if (subscription == &onoffbutton) {
      Serial.print(F("Got: "));
      Serial.println((char *)onoffbutton.lastread);
    }
    if (subscription == &media_title) {
      song_title = ((char *)media_title.lastread);
      Serial.println(song_title.length());
      song_title.remove(0,1);
      song_title.remove((song_title.length() - 1),1);
      Serial.println(song_title.length());
      Serial.println(song_title);
//      Serial.println((char *)media_title.lastread);
//      song_title = ((char *)media_title.lastread);
//      testscrolltext((char *)media_title.lastread);
      song_title.toCharArray(title,song_title.length()+1);
      Serial.println(title);
//      testscrolltext(song_title);
    } 
  }
}

void Task1code( void * pvParameters ){
  for(;;){
    scrollText(title);
  } 
}

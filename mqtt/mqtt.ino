#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "secrets.h"
/************************* WiFi Access Point *********************************/

//#define WLAN_SSID       ""//Defined in secrets.h
//#define WLAN_PASS       ""//Defined in secrets.h

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
//#define AIO_USERNAME  ""//Defined in secrets.h
//#define AIO_KEY       ""//Defined in secrets.h

/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

Adafruit_MQTT_Subscribe feed_time = Adafruit_MQTT_Subscribe(&mqtt, "time/seconds");

Adafruit_MQTT_Subscribe feed_pulse_ms = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/pulse_ms", MQTT_QOS_1);

Adafruit_MQTT_Subscribe feed_onoff0 = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/onoff0", MQTT_QOS_1);
Adafruit_MQTT_Subscribe feed_onoff1 = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/onoff1", MQTT_QOS_1);

Adafruit_MQTT_Subscribe feed_pulse0 = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/pulse0", MQTT_QOS_1);
Adafruit_MQTT_Subscribe feed_pulse1 = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/pulse1", MQTT_QOS_1);

/*************************** Sketch Code ************************************/


#define pin_onoff_prod 4
#define pin_onoff_preprod 5



int seconds;
int minutes;
int hours;

int timeZone = 0; // UTC 

void timecallback(uint32_t current) {

  // adjust to local time zone
  current += (timeZone * 60 * 60);

  // calculate current time
  seconds = current % 60;
  current /= 60;
  minutes = current % 60;
  current /= 60;
  hours = current % 24;

  // print hours
  if(hours == 0 || hours == 12)
    Serial.print("12");
  if(hours < 12)
    Serial.print(hours);
  else
    Serial.print(hours - 12);

  // print minutess
  Serial.print(":");
  if(minutes < 10) Serial.print("0");
  Serial.print(minutes);

  // print secondsonds
  Serial.print(":");
  if(seconds < 10) Serial.print("0");
  Serial.print(seconds);

  if(hours < 12)
    Serial.println(" am");
  else
    Serial.println(" pm");
}

unsigned short int pulse_ms = 100;
bool onoff[2] = {false, false};
bool last_pulse_onoff [2] = {false, false};
unsigned long next_pulse_evt_ms [2] = {0LL, 0LL};

void callback_slider(double x) {
  if ((x * 1000) > 65535) x = 65535.0;
  if (x < 0) x = 0.0;
  pulse_ms = (unsigned short)(x * 1000.0); 
}

void handle_onoff(int chan, char *data) {
  char ch;
  char val = data[0] - '0';
  if (chan < 0 || chan > 1) return;
  if (val < 0 || val > 1) return;

  onoff[chan] = val==1?true:false;

  ch = '0' + chan;

  Serial.print("Onoff updated on chan ");
  Serial.print(ch);
  Serial.print(" Now ");
  Serial.println(val?"On":"Off");
}

void callback_onoff0(char *data, uint16_t len) {
  handle_onoff(0, data);
}

void callback_onoff1(char *data, uint16_t len) {
  handle_onoff(1, data);
}


void handle_pulse(int chan) {
  char ch;
  unsigned long long now = millis();
  unsigned long long next = now + pulse_ms;
  if (pulse_ms == 0L) return;
  if (chan < 0 || chan > 1) return;

  last_pulse_onoff[chan] = onoff[chan];

  if (next > 0xFFFFFFFFL) next -= 0xFFFFFFFFL;

  next_pulse_evt_ms[chan] = (unsigned long)next;
  
  ch = '0' + chan;

  Serial.print("Pulse ");
  Serial.print(pulse_ms);
  Serial.print(" requested on: chan ");
  Serial.println(ch);
}

void callback_pulse0(char *data, uint16_t len) {
  handle_pulse(0);
}

void callback_pulse1(char *data, uint16_t len) {
  handle_pulse(1);
}

void setupIO()
{
  pinMode(pin_onoff_preprod, OUTPUT);
  pinMode(pin_onoff_prod, OUTPUT);
  digitalWrite(pin_onoff_preprod, LOW);
  digitalWrite(pin_onoff_prod, LOW);
}


void setup() {

  setupIO();

  Serial.begin(115200);
  delay(10);

  Serial.println(F("Remote USB Unplug over MQTT"));

  // Connect to WiFi access point.
  Serial.println();
  Serial.println();
  Serial.print("Connecting to WIFI: ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WIFI Connected, IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.subnetMask());
  Serial.println(WiFi.gatewayIP());
  Serial.println(WiFi.dnsIP());
 
  Serial.println();

  feed_time.setCallback(timecallback);
  feed_pulse0.setCallback(callback_pulse0);
  feed_pulse0.setCallback(callback_pulse1);
  feed_onoff0.setCallback(callback_onoff0);
  feed_onoff1.setCallback(callback_onoff1);

  
  // Setup MQTT subscription for time feed.
  mqtt.subscribe(&feed_time);
  mqtt.subscribe(&feed_pulse0);
  mqtt.subscribe(&feed_pulse1);
  mqtt.subscribe(&feed_onoff0);
  mqtt.subscribe(&feed_onoff1);


}

//uint32_t x=0;

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  // this is our 'wait for incominutesg subscription packets and callback em' busy subloop
  // try to spend your time here:
  mqtt.processPackets(10000);
  
  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE secondsonds
  
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }

  for(unsigned int chan=0;chan<1;chan++) {
    if (next_pulse_evt_ms[chan] < millis()){
      digitalWrite(pin_onoff_preprod, onoff[chan]);
    } else {
      digitalWrite(pin_onoff_preprod, ~last_pulse_onoff[chan]);
    }
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
       Serial.println("Retrying MQTT connection in 10 secondsonds...");
       mqtt.disconnect();
       delay(10000);  // wait 10 secondsonds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}

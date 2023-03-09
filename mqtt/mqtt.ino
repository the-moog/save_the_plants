
#define MQTT_DEBUG

#define USE_WIFI_NETWORK
#define USE_WEBSOCKETS

#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>


#ifndef USE_WIFI_NETWORK
#include <ArduinoSerialToTCPBridgeClient.h>
#else
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#endif
#ifdef USE_WEBSOCKETS
#include "WebSocketStreamClient.h"
#endif

#include "certs.h"
//#include "Adafruit_MQTT_Client.h"
#include "secrets.h"
/************************* WiFi Access Point *********************************/

//#define WLAN_SSID       ""//Defined in secrets.h
//#define WLAN_PASS       ""//Defined in secrets.h

/************************* Adafruit.io Setup *********************************/

//#define AIO_SERVER      MQTT_HOST
//"io.adafruit.com"

//#ifndef USE_WEBSOCKETS
//#define AIO_SERVERPORT  8884
//1883
//#else
//#define AIO_SERVERPORT  443
//#endif
//#define AIO_USERNAME  ""//Defined in secrets.h
//#define AIO_KEY       ""//Defined in secrets.h

#ifdef USE_WIFI_NETWORK
#define Serial_print(...) Serial.print(__VA_ARGS__)
#else
#define Serial_print(...)
#endif

#ifdef USE_WIFI_NETWORK
#define Serial_println(...) Serial.println(__VA_ARGS__)
#else
#define Serial_print(...)
#endif
/************ Global State (you don't need to change this!) ******************/
/*
NOTE ON SSL:
For TLS or SSL to work (i.e. https) you need a server certificate
use the command ..\certUpdate.bat from within this project folder
This reads .\secrets.h and creates .\certs.h to match

*/


// Create an ESP8266 WiFiClient class to connect to the MQTT server.
#ifdef USE_WIFI_NETWORK
WiFiClientSecure * net;
#define SERIAL_BAUD 115200
#else
ArduinoSerialToTCPBridgeClient net;
#define SERIAL_BAUD 921600
#endif

#ifdef USE_WEBSOCKETS
WebSocketClient * ws;
WebSocketStreamClient * ws_client;
#endif

//Adafruit_MQTT_Client * mqtt;
PubSubClient * mqttClient;

#ifdef USE_WEBSOCKETS

/*
void onMessageCallback(WebsocketsMessage message) {
    Serial_print("Got Message: ");
    Serial_println(message.data());
}

void onEventsCallback(WebsocketsEvent event, String data) {
    if(event == WebsocketsEvent::ConnectionOpened) {
        Serial_println("Connnection Opened");
    } else if(event == WebsocketsEvent::ConnectionClosed) {
        Serial_println("Connnection Closed");
    } else if(event == WebsocketsEvent::GotPing) {
        Serial_println("Got a Ping!");
    } else if(event == WebsocketsEvent::GotPong) {
        Serial_println("Got a Pong!");
    }
}
*/
#endif


/*************************** Sketch Code ************************************/


#define pin_onoff_prod 4
#define pin_onoff_preprod 5

X509List cert(Root_CA);

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
    Serial_print("12");
  if(hours < 12)
    Serial_print(hours);
  else
    Serial_print(hours - 12);

  // print minutess
  Serial_print(":");
  if(minutes < 10) Serial_print("0");
  Serial_print(minutes);

  // print secondsonds
  Serial_print(":");
  if(seconds < 10) Serial_print("0");
  Serial_print(seconds);

  if(hours < 12)
    Serial_println(" am");
  else
    Serial_println(" pm");
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

  Serial_print("Onoff updated on chan ");
  Serial_print(ch);
  Serial_print(" Now ");
  Serial_println(val?"On":"Off");

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

  Serial_print("Pulse ");
  Serial_print(pulse_ms);
  Serial_print(" requested on: chan ");
  Serial_println(ch);

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

/*
Adafruit_MQTT_Subscribe * feed_time;
Adafruit_MQTT_Subscribe * feed_pulse_ms;
Adafruit_MQTT_Subscribe * feed_onoff0;
Adafruit_MQTT_Subscribe * feed_onoff1;
Adafruit_MQTT_Subscribe * feed_pulse0;
Adafruit_MQTT_Subscribe * feed_pulse1; 

void setupSubscriptions() {
  feed_time = new Adafruit_MQTT_Subscribe(mqtt, "time/seconds");
  feed_pulse_ms = new Adafruit_MQTT_Subscribe(mqtt, AIO_USERNAME "/feeds/pulse_ms", MQTT_QOS_1);
  feed_onoff0 = new Adafruit_MQTT_Subscribe(mqtt, AIO_USERNAME "/feeds/onoff0", MQTT_QOS_1);
  feed_onoff1 = new Adafruit_MQTT_Subscribe(mqtt, AIO_USERNAME "/feeds/onoff1", MQTT_QOS_1);
  feed_pulse0 = new Adafruit_MQTT_Subscribe(mqtt, AIO_USERNAME "/feeds/pulse0", MQTT_QOS_1);
  feed_pulse1 = new Adafruit_MQTT_Subscribe(mqtt, AIO_USERNAME "/feeds/pulse1", MQTT_QOS_1);

  feed_time->setCallback(timecallback);
  feed_pulse0->setCallback(callback_pulse0);
  feed_pulse0->setCallback(callback_pulse1);
  feed_onoff0->setCallback(callback_onoff0);
  feed_onoff1->setCallback(callback_onoff1);
}
*/


bool mqttConnect() {
  // Connect to server
  //ws->connectSecure(AIO_SERVER, 443, "/");
  ESP.wdtFeed();
  // Send a message
  //ws->send("Hi Server!");
  // Send a ping
  //ws->ping();
  mqttClient->setCallback(MQTTcallback);
  Serial_println();

  String uid = "QDEVSYS" + String((unsigned long long)net);

  Serial.print("UID: ");
  Serial.println(uid);

  bool connected = mqttClient->connect(uid.c_str(), AIO_USERNAME, AIO_KEY);
  // Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
  //mqtt = new Adafruit_MQTT_Client(ws_client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_USERNAME, AIO_KEY);

  //setupSubscriptions();

  //MQTT_connect();
  if(connected) {
    Serial_println("Connected");
    Serial_println("Subscribing to #");
    mqttClient->subscribe("#");
  } else {
    Serial_println("ws connect fail");
  }

  ESP.wdtFeed();
  Serial_print("HTTP State: ");
  Serial_println(ws->responseStatusCode());
  Serial_print("MQTT State: ");
  Serial_println(mqttClient->state());
  return connected && mqttClient->state() == MQTT_CONNECTED;
}

void setupNTP() {
  // Set time via NTP, as required for x.509 validation
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

void setup() {

  setupIO();

  Serial.begin(SERIAL_BAUD);
  delay(5000);
  ESP.wdtFeed();

  Serial_println("\n\nRemote USB Unplug over MQTT");
  Serial_println();


#ifdef USE_WIFI_NETWORK
// Connect to WiFi access point.
  Serial_println();
  Serial_print("Connecting to WIFI: ");
  Serial_println(WLAN_SSID);
  net = new WiFiClientSecure();
  //net->setInsecure();

  WiFi.begin(WLAN_SSID, WLAN_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial_print(".");
    ESP.wdtFeed();
  }
  Serial_println();
  Serial_println("WIFI Connected, IP address: ");
  Serial_println(WiFi.localIP());
  Serial_println(WiFi.subnetMask());
  Serial_println(WiFi.gatewayIP());
  Serial_println(WiFi.dnsIP());
  Serial_println();
#endif

  setupNTP();

  Serial.print("Connecting to ");
  Serial.println(myhost_fqdn);

  Serial.printf("Using certificate: %s\n", Root_CA);
  net->setTrustAnchors(&cert);

  ws = new WebSocketClient(*net, myhost_fqdn, myhost_port);
  Serial_print("HTTP State: ");
  Serial_println(ws->responseStatusCode());
  ws_client = new WebSocketStreamClient(*ws, "mqtt");
  mqttClient = new PubSubClient(*ws_client);
  Serial_print("MQTT State: ");
  Serial_println(mqttClient->state());

  // Setup Callbacks
  //ws->onMessage(onMessageCallback);
  //ws->onEvent(onEventsCallback);
  ///client.setClient(*s);

  mqttConnect();



  // Setup MQTT subscription for time feed.
  /*
  mqtt->subscribe(feed_time);
  mqtt->subscribe(feed_pulse0);
  mqtt->subscribe(feed_pulse1);
  mqtt->subscribe(feed_onoff0);
  mqtt->subscribe(feed_onoff1);
*/


}



void loop() {
  ESP.wdtFeed();
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  //ws->poll();

  //MQTT_connect();

  // this is our 'wait for incominutesg subscription packets and callback em' busy subloop
  // try to spend your time here:
  //mqtt->processPackets(10000);
  
  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE secondsonds
  
  //if(! mqtt->ping()) {
  //  mqtt->disconnect();
  //}

  if (!mqttClient->connected()) reconnectMQTT();

  mqttClient->loop();

  for(unsigned int chan=0;chan<1;chan++) {
    if (next_pulse_evt_ms[chan] < millis()){
      digitalWrite(pin_onoff_preprod, onoff[chan]);
    } else {
      digitalWrite(pin_onoff_preprod, ~last_pulse_onoff[chan]);
    }
  }
}


/*
// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Already connected.
  if (mqtt->connected())return;

  Serial_print("Connecting to MQTT->.. ");

  uint8_t retries = 3;
  while ((ret = mqtt->connect()) != 0) { // connect will return 0 for connected
    Serial_println(mqtt->connectErrorString(ret));
    Serial_println("Retrying MQTT connection in 10 secondsonds...");

    mqtt->disconnect();
    delay(10000);  // wait 10 secondsonds
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      while (1);
    }
  }
  Serial_println("MQTT Connected!");
}
*/


void reconnectMQTT()
{
  while (!mqttClient->connected())
  {
    Serial_print("Attempting MQTT connection...");

    if (! mqttConnect()) {
      Serial_print("failed, rc=");
      Serial_print(mqttClient->state());
      Serial_println(" try again in 5 seconds");

      for(int n=0;n<50;n++) {
        ESP.wdtFeed();
        delay(100);
      }
    }
  }
}


void MQTTcallback(char *topic, byte *payload, unsigned int length)
{
    Serial_print("Payload: ");
    for (int i = 0; i < length; i++)
        Serial_print((char)payload[i]);

    Serial_println();
}

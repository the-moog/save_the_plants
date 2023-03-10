
#define USE_WIFI_NETWORK
#define USE_WEBSOCKETS
#define USE_SSL

#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>
#include "secrets.h"

#define ROMSTR(v,s) const typeof(*s) * v PROGMEM = s

#ifndef USE_WIFI_NETWORK
#include <ArduinoSerialToTCPBridgeClient.h>
#else
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#ifdef USE_SSL
#include <WiFiClientSecureBearSSL.h>
#endif
#endif

#ifdef USE_WEBSOCKETS
#include "WebSocketStreamClient.h"
#endif
#ifdef USE_SSL
#include "certs.h"
#else
ROMSTR(myhost_fqdn, MQTT_HOST);
const uint16_t myhost_port = MQTT_PORT;
#endif

//#include "Adafruit_MQTT_Client.h"
//#define tmpvar var_##__LINE__
//#define MSG(x) {romstr_t tmpvar = x;Serial.write(tmpvar);}
ROMSTR(msg_connected, "Connected\r\n");
ROMSTR(msg_crlf, "\r\n");
ROMSTR(msg_null, "");

const char * http_error_str [] PROGMEM = {
  "SUCCESS",
  "CONNECTION FAILED",
  "API ERROR",
  "TIMED OUT",
  "INVALID RESPONSE",
};

const int http_error_count = sizeof(http_error_str)/sizeof(const char *);

#ifdef USE_WIFI_NETWORK
#define MSG(x) Serial.write(x)
#else
#define MSG(x) 
#endif

#define MSG_CRLF MSG(msg_crlf)
#define MSG_CONNECTED MSG(msg_connected)

#ifdef USE_WIFI_NETWORK
#define Serial_printf(...) Serial.printf(__VA_ARGS__)
#else
#define Serial_printf(...)
#endif

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

/*
NOTE ON SSL:
For TLS or SSL to work (i.e. https) you need a server certificate
use the command ..\certUpdate.bat from within this project folder
This reads .\secrets.h and creates .\certs.h to match
*/


#ifdef USE_WIFI_NETWORK
#ifdef USE_SSL
WiFiClientSecure net;
#else
WiFiClient net;
#endif
#define SERIAL_BAUD 115200
#else
ArduinoSerialToTCPBridgeClient net;
#define SERIAL_BAUD 921600
#endif

const char * get_http_error(int error) {
  if (error > 0 || error <= -http_error_count) {
    Serial_printf("Unknown HTTP err: %d\n", error);
    return msg_null;
  }
  return http_error_str[-error];
}

WebSocketClient ws(net, myhost_fqdn, myhost_port);
WebSocketStreamClient ws_client(ws, "/mqtt");
PubSubClient mqttClient(ws_client);

#define pin_onoff_prod 4
#define pin_onoff_preprod 5

#ifdef USE_SSL
X509List cert(Root_CA);
#endif

static unsigned long long next_connect = 0LL;
static unsigned short int pulse_ms = 100;
static bool onoff[2] = {false, false};
static bool last_pulse_onoff [2] = {false, false};
static unsigned long long next_pulse_evt_ms [2] = {0LL, 0LL};

void callback_slider(double x) {
  if ((x * 1000) > 65535) x = 65535.0;
  if (x < 0) x = 0.0;
  pulse_ms = (unsigned short)(x * 1000.0); 
}

void handle_onoff(int chan, char *data) {
  char ch;
  signed char val = data[0] - '0';
  if (chan < 0 || chan > 1) return;
  if (val < 0 || val > 1) return;

  onoff[chan] = val==1?true:false;

  ch = '0' + chan;

  MSG("Onoff updated on chan ");
  Serial_print(ch);
  MSG(" Now ");
  MSG(val?"On":"Off");
  MSG_CRLF;

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

  MSG("Pulse ");
  Serial_print(pulse_ms);
  MSG(" requested on: chan ");
  Serial_print(ch);
  MSG_CRLF;

}

void callback_pulse0(char *data, uint16_t len) {
  (void)data;
  (void)len;
  handle_pulse(0);
}

void callback_pulse1(char *data, uint16_t len) {
  (void)data;
  (void)len;
  handle_pulse(1);
}

void setupIO()
{
  pinMode(pin_onoff_preprod, OUTPUT);
  pinMode(pin_onoff_prod, OUTPUT);
  digitalWrite(pin_onoff_preprod, LOW);
  digitalWrite(pin_onoff_prod, LOW);
}


ROMSTR(uid, "QDEFSYS");
int mqttConnect() {
  wdt_reset();

  int current_state = mqttClient.state();

  if (next_connect > millis())return current_state;
  next_connect = millis() + 10000LL;

  if(current_state == MQTT_CONNECTED) {
    //ws->ping();
    return MQTT_CONNECTED;
  }
  MSG("Connecting MQTT:\r\n");
  mqttClient.setCallback(MQTTcallback);
  bool connected = mqttClient.connect(uid, MQTT_USERNAME, MQTT_KEY);
  if (connected) {
    MSG_CONNECTED;
    mqttClient.subscribe("test");
  } else {
    MSG("Fail: HTTP State: ");
    int http_status = ws.responseStatusCode();
    Serial_printf("%d: %s\n", http_status, get_http_error(http_status));
  }
  wdt_reset();

  MSG("MQTT Old State: ");
  Serial_println(current_state);
  current_state = mqttClient.state();
  MSG("MQTT New State: ");
  Serial_println(current_state);

  return current_state;
}

void delay_s(unsigned long ms) {
  unsigned long count = ms / 100;
  for ( ; count > 0; count--) {
    delay(100);
    wdt_reset();
  }
}

ROMSTR(ntp_server0, "pool.ntp.org");
ROMSTR(ntp_server1, "time.nist.gov");
void setupNTP() {
  // Set time via NTP, as required for x.509 validation
  configTime(3 * 3600, 0, ntp_server0, ntp_server1);

  MSG("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay_s(500);
    MSG(".");
    now = time(nullptr);
  }
  MSG_CRLF;
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  MSG("Current time: ");
  Serial.print(asctime(&timeinfo));
}


void setupWiFi() {

  MSG_CRLF;
  MSG("Connecting to WIFI: ");
  //Serial_println(WLAN_SSID);

  //net->setInsecure();

  WiFi.begin(WLAN_SSID, WLAN_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay_s(500);
    MSG(".");
  }
  MSG_CRLF;
  MSG("WIFI Connected, IP address: \r\n");
  Serial_println(WiFi.localIP());
  Serial_println(WiFi.subnetMask());
  Serial_println(WiFi.gatewayIP());
  Serial_println(WiFi.dnsIP());
  MSG_CRLF;
}


void setupHost() {
  Serial.printf("Connecting to %s:%u\r\n",myhost_fqdn,myhost_port);
#ifdef USE_SSL
  //Serial.printf("Using certificate: %s\n", Root_CA);
  net.setTrustAnchors(& cert);
#endif
}

void setup() {
  wdt_disable();
  wdt_enable(WDTO_8S);
  setupIO();

  Serial.begin(SERIAL_BAUD);
  delay_s(5000);
  MSG_CRLF;
  Serial.println(ESP.getResetReason());
  Serial.println(ESP.getResetInfo());

  MSG("\n\nRemote USB Unplug over MQTT\r\n");


#ifdef USE_WIFI_NETWORK
  setupWiFi();
#endif

  setupNTP();
  setupHost();


}

void relayLoop() {
  for(unsigned int chan=0;chan<1;chan++) {
    if (next_pulse_evt_ms[chan] < millis()){
      digitalWrite(pin_onoff_preprod, onoff[chan]);
    } else {
      digitalWrite(pin_onoff_preprod, ~last_pulse_onoff[chan]);
    }
  }
}

// Main work loop
void loop() {

  mqttConnect();

  mqttClient.loop();
  //relayLoop()
  wdt_reset();
}


void MQTTcallback(char *topic, byte *payload, unsigned int length)
{
    Serial_printf("Topic: %s : Payload: \r\n", topic);
    for (unsigned i = 0; i < length; i++)
        Serial_print((char)payload[i]);
    MSG_CRLF;
}

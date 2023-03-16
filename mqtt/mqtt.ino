
#include "mqtt_defs.h"



#ifndef USE_SSL
ROMSTR(myhost_fqdn, MQTT_HOST);
const uint16_t myhost_port = MQTT_PORT;
#endif

ROMSTR(msg_connected, "Connected\r\n");
ROMSTR(msg_crlf, "\r\n");
ROMSTR(msg_null, "");

#ifdef USE_HTTP_ERR_PRETTY
const char * http_error_str [] PROGMEM = {
  "SUCCESS",
  "CONNECTION FAILED",
  "API ERROR",
  "TIMED OUT",
  "INVALID RESPONSE",
};
const int http_error_count = sizeof(http_error_str)/sizeof(const char *);
#endif



#define MSG_CRLF MSG(msg_crlf)
#define MSG_CONNECTED MSG(msg_connected)



/*
NOTE ON SSL:
For TLS or SSL to work (i.e. https) you need a server certificate
use the command ..\certUpdate.bat (or ../certUpdate.sh) from within
this project folder
This reads .\secrets.h and creates a .\certs.h to match
*/


#ifdef USE_WIFI_NETWORK
  #ifdef USE_SSL
   WiFiClientSecure netClient;
  #else
   WiFiClient netClient;
  #endif
  #define SERIAL_BAUD 115200
 #else
  ArduinoSerialToTCPBridgeClient netClient;
  #define SERIAL_BAUD 921600
#endif

#ifdef USE_HTTP_ERR_PRETTY
const char * get_http_error(int error) {
  if (error > 0 || error <= -http_error_count) {
    Serial_printf("Unknown HTTP err: %d\n", error);
    return msg_null;
  }
  return http_error_str[-error];
}
#endif

WebSocketClient wsClient(netClient, myhost_fqdn, myhost_port);
WebSocketStreamClient wsStream(wsClient, "/mqtt");
PubSubClient mqttClient(wsStream);
PubSubClientTools mqtt(mqttClient);

#ifdef USE_MQTT_PUBLISHER
ThreadController threadControl = ThreadController();
Thread thread = Thread();
#endif

#define pin_onoff_prod 4
#define pin_onoff_preprod 5

#ifdef USE_SSL
X509List cert(Root_CA);
#endif

static unsigned long long next_connect = 0ULL;
static unsigned short int pulse_ms = 100;
static bool onoff[2] = {false, false};
static unsigned long long next_pulse_evt_ms [2] = {0ULL, 0ULL};

// Helper to parse the REST like MQTT message for the channel number
int get_chan(const std::string & text) {
  const char * value = text.substr(text.rfind('/')+1).c_str();
  if (*value != '0' && *value != '1')return -1;
  int chan = ((signed char)(*value)) - '0';
  return chan;
}

// Called by MQTT
void callback_pulse_ms(const std::string & topic, const std::string & msg) {
  (void)topic;
  double value = 0.0;
  try {
    value = std::stof(msg.c_str());
  } catch (std::logic_error const& ex) {
    Serial_printf("Bad Message: %s\n", msg.c_str());
    return;
  }
  if ((value * 1000) > 65535) value = 65535.0;
  if (value < 0) value = 0.0;
  pulse_ms = (unsigned short)(value * 1000.0); 
}

// Called by MQTT
void callback_onoff(const std::string & topic, const std::string & msg) {
  int chan = get_chan(topic);
  if (chan < 0)return;
  
  handle_onoff(chan, msg);
}

void callback_pulse(const std::string & topic, const std::string & msg) {
  int chan = get_chan(topic);
  if (chan < 0)return;
  handle_pulse(chan, msg);
}

// Handle the messages for onoff
void handle_onoff(int chan, const std::string & msg) {
  signed char val = msg[0] - '0';
  if (chan < 0 || chan > 1) return;
  if (val < 0 || val > 1) return;
  bool prev = onoff[chan];
  onoff[chan] = val==1?true:false;

  // Nothing to do
  if (onoff[chan] == prev)return;

  Serial_printf("Relay chan %d -> %s\n", chan, val?"On":"Off");
}

// Requesting a pulse either makes a pulse now (on->off-> or off->on->off)
// or extends an existing pulse
void handle_pulse(int chan, const std::string & msg) {
  (void)msg;

  unsigned long long now = millis();
  unsigned long long next = now + pulse_ms;
  bool pulse_active = next_pulse_evt_ms[chan] != 0L;

  if (pulse_ms == 0L) return;
  if (chan < 0 || chan > 1) return;

  // Handle overflows as millis() wrap every 50 days or so
  if (next > 0xFFFFFFFFL) next -= 0xFFFFFFFFL;

  // If the result is next == 0ULL then make it 1 as
  // zero is used for never.
  if (next == 0ULL) next= 1ULL;

  next_pulse_evt_ms[chan] = (unsigned long)next;
  if (! pulse_active ) onoff[chan] = ! onoff[chan];
  
  Serial_printf("%3.4fs pulse triggered on chan %d\r\n", pulse_ms / 1000.0, chan);
}


// Setup the IO pins for the relays
void setupIO()
{
  pinMode(pin_onoff_preprod, OUTPUT);
  pinMode(pin_onoff_prod, OUTPUT);
  digitalWrite(pin_onoff_preprod, LOW);
  digitalWrite(pin_onoff_prod, LOW);
}


// This makes the initial MQTT connection
// by upgrading a HTTPS session to a WSS session

int mqttConnect() {
  wdt_reset();

  int current_state = mqttClient.state();
  int new_state=current_state;

  if (next_connect > millis())return current_state;
  next_connect = millis() + 10000ULL;

  if(current_state == MQTT_CONNECTED) {
    //ws->ping();
    return MQTT_CONNECTED;
  }
  MSG("Connecting MQTT:\r\n");
  const String uid="QDEVSYS_"+WiFi.macAddress();
  bool connected = mqtt.connect(String(uid), String(F(MQTT_USERNAME)), String(F(MQTT_KEY)));
  if (connected) {
    Serial_printf("Connected: ID=%s\n", uid.c_str());

    mqtt.subscribe(String(F("cfg/pulse_s")), callback_pulse_ms);
    /*
    mqtt.subscribe(F("dev/onoff/0"), MAKE_CALLBACK(callback_onoff));
    mqtt.subscribe(F("dev/onoff/1"), MAKE_CALLBACK(callback_onoff));
    mqtt.subscribe(F("dev/pulse/0"), MAKE_CALLBACK(callback_pulse));
    mqtt.subscribe(F("dev/pulse/1"), MAKE_CALLBACK(callback_pulse));
    */
  } else {
    MSG("Connect Fail\r\n");
  }
  wdt_reset();
  new_state = mqttClient.state();

  Serial_printf("MQTT State: %d -> %d\n",current_state, new_state);

  return current_state;
}

// Use this for longer delays to prevent the watchdog
// kicking in
void delay_s(unsigned long ms) {
  unsigned long count = ms / 100;
  for ( ; count > 0; count--) {
    delay(100);
    wdt_reset();
  }
}

String getRealTime(){
  String timestr;
  time_t now = time(nullptr);
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  timestr = asctime(&timeinfo);
  return timestr;
}

// Connect to NTP
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
  MSG("Current time: ");
  Serial.print(getRealTime());
}

// Connect to WIFI
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
  Serial.printf("Connecting to %s:%u\r\n", myhost_fqdn, myhost_port);

#ifdef USE_SSL
  Serial.printf("Using certificate: %s\n", Root_CA_cn);
  netClient.setTrustAnchors(& cert);
#endif
}

void setup() {
  wdt_disable();
  wdt_enable(WDTO_8S);
  setupIO();

  Serial.begin(SERIAL_BAUD);
  delay_s(5000);
  MSG_CRLF;

  MSG("Reset Reason:\r\n");
  Serial.println(ESP.getResetReason());
  Serial.println(ESP.getResetInfo());

  MSG("\n\nRemote USB Unplug over MQTT\r\n");

#ifdef USE_WIFI_NETWORK
  setupWiFi();
#endif

  // NTP is required for https / wss
  setupNTP();
  setupHost();

#ifdef USE_MQTT_PUBLISHER
  // Enable Thread
  thread.onRun(pub_ntp_time);
  thread.setInterval(10000);
  threadControl.add(&thread);
#endif
}

// Update the relay states
void relayLoop() {
  for(unsigned int chan=0;chan<1;chan++) {
    if ((millis() > next_pulse_evt_ms[chan]) && (next_pulse_evt_ms[chan] !=0L)) {
      onoff[chan] = !onoff[chan];
      next_pulse_evt_ms[chan] = 0L;
    }
    digitalWrite(pin_onoff_preprod, onoff[chan]);
  }
}

// Main work loop
void loop() {

  // Keep connecting, the underlying system handles
  // actual connections as required
  mqttConnect();

  // Keep processing MQTT messages
  mqttClient.loop();

  // Keep updating the relay
  relayLoop();

  //  Feed the dog, just in case
  wdt_reset();

  #ifdef USE_MQTT_PUBLISHER
    // Publish data on a thread
    threadControl.run();
  #endif
}

#ifdef USE_MQTT_PUBLISHER
void pub_ntp_time() {
  String time = getRealTime();
  Serial_printf("pub <time>: %s\n",time.c_str());
  mqtt.publish("time", time);
}
#endif

/*
// Callback used if we are not using the PubSubClientTools
void MQTTcallback(char *topic, byte *payload, unsigned int length)
{
    Serial_printf("Topic: %s : Payload: \r\n", topic);
    for (unsigned i = 0; i < length; i++)
        Serial_print((char)payload[i]);
    MSG_CRLF;
}
*/

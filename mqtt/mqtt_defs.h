#pragma once

#include "mqtt_app.h"

#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>
#include <PubSubClientTools.h>
#include <string>
#include <stdexcept>


// Ensure the compiler does not get the storage wrong
// for strings as otherwise they take up a lot of our 4k stack
#define ROMSTR(v,s) const typeof(*s) * v PROGMEM = s

#ifndef USE_WIFI_NETWORK
 #include <ArduinoSerialToTCPBridgeClient.h>
#else
 #include <ESP8266WiFiMulti.h>
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
#endif

#ifdef USE_MQTT_PUBLISHER
 #include <Thread.h>
 #include <ThreadController.h>
#endif

#ifdef USE_WIFI_NETWORK
 #define MSG(x) Serial.write(x)
#else
 #define MSG(x) 
#endif

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


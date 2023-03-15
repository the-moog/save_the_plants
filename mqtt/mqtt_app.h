#include "secrets.h"

#define USE_WIFI_NETWORK
#undef USE_MQTT_PUBLISHER
#undef USE_HTTP_ERR_PRETTY

#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>
#include <PubSubClientTools.h>
#include <string>
#include <stdexcept>
	

#ifdef USE_MQTT_PUBLISHER
#include <Thread.h>
#include <ThreadController.h>
#endif

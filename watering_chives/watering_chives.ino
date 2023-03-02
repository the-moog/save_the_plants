
#define BLYNK_TEMPLATE_ID "TMPL0yNVWWUx"
#define BLYNK_DEVICE_NAME "Chives"
#define BLYNK_AUTH_TOKEN BLYNK_TOKEN_1
#define BLYNK_FIRMWARE_VERSION "0.2.2"

//#define BLYNK_DEBUG 
// Comment this out to disable prints and save space
#define BLYNK_PRINT Serial

// NOTE This must be included after the BLYNK definitions
// otherwise it fails to connect
#include <BlynkSimpleEsp8266.h>

const char auth[] = BLYNK_AUTH_TOKEN;


BLYNK_WRITE(V0)
{
  blynk_write_V0(param.asInt());
}

BLYNK_WRITE(V5)
{
  blynk_write_V5(param.asInt());
}

BLYNK_WRITE(V7)
{
  blynk_write_V7(param.asInt());
}


BLYNK_WRITE(V8)
{
  blynk_write_V8(param.asInt());
}

BLYNK_CONNECTED()
{
  blynk_connected();
}

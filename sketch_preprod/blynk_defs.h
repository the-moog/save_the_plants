/*
This file includes the Blynp API headers and
is where the various macros required for the template
are placed
*/


// NOTE This must be included after the BLYNK definitions
// otherwise it fails to connect
#include <BlynkSimpleEsp8266.h>


extern bool pumpOverride;
extern unsigned wateringDuration;


// This function is called every time the Virtual Pin 0 state changes
BLYNK_WRITE(V0)
{
  // Set incoming value from pin V0 to a variable
  int value = param.asInt();

  // Update state
  Blynk.virtualWrite(V1, value);
}

// This function is called every time the Virtual Pin 5 changes state
BLYNK_WRITE(V5)
{
  pumpOverride = param.asInt()>0?true:false;

  if(!pumpOverride)wateringDuration = 0;
}


// This function is called every time the device is connected to the Blynk.Cloud
BLYNK_CONNECTED()
{
  // Change Web Link Button message to "Congratulations!"
  Blynk.setProperty(V3, "offImageUrl", "https://static-image.nyc3.cdn.digitaloceanspaces.com/general/fte/congratulations.png");
  Blynk.setProperty(V3, "onImageUrl",  "https://static-image.nyc3.cdn.digitaloceanspaces.com/general/fte/congratulations_pressed.png");
  Blynk.setProperty(V3, "url", "https://docs.blynk.io/en/getting-started/what-do-i-need-to-blynk/how-quickstart-device-was-made");
}

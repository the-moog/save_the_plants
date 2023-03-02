#ifndef __APP_H__
#define __APP_H__

//Generated from secrets.tmplh
#include "secrets.h"

#include "hw_defs.h"

#define USB_SW_TEMPLATE "TMPLjlHMNf6L"

#define BLYNK_TEMPLATE_ID USB_SW_TEMPLATE
#define BLYNK_DEVICE_NAME "preprod"
#define BLYNK_AUTH_TOKEN BLYNK_TOKEN_7
#define USE_SCHEDULE 0


// Comment this out to disable prints and save space
#define BLYNK_PRINT Serial
//#define BLYNK_DEBUG 

#include "blynk_defs.h"

#endif
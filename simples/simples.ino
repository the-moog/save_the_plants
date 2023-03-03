

/*
   -- New project --
   
   This source code of graphical user interface 
   has been generated automatically by RemoteXY editor.
   To compile this code using RemoteXY library 3.1.8 or later version 
   download by link http://remotexy.com/en/library/
   To connect using RemoteXY mobile app by link http://remotexy.com/en/download/                   
     - for ANDROID 4.11.1 or later version;
     - for iOS 1.9.1 or later version;
    
   This source code is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.    
*/

//////////////////////////////////////////////
//        RemoteXY include library          //
//////////////////////////////////////////////

// RemoteXY select connection mode and include library 
#define REMOTEXY_MODE__WIFI_CLOUD

// RemoteXY connection settings 
#define REMOTEXY_WIFI_SSID "Hydra"
#define REMOTEXY_WIFI_PASSWORD "K5x48Vz3"
#define REMOTEXY_CLOUD_SERVER "cloud.remotexy.com"
#define REMOTEXY_CLOUD_PORT 6376
#define REMOTEXY_CLOUD_TOKEN "0d15d9a763d644975b4789cf2f8d9ce0"

#define REMOTEXY__DEBUGLOG

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Async_Operations.h>

#include "event_detect.h"

#include <RemoteXY.h>


// RemoteXY configurate  
#pragma pack(push, 1)
uint8_t RemoteXY_CONF[] =   // 93 bytes
  { 255,4,0,51,0,86,0,16,177,1,10,48,8,25,15,15,4,26,31,79,
  78,0,31,79,70,70,0,2,1,8,51,14,9,2,26,31,31,79,78,47,
  79,70,70,0,80,85,76,83,69,0,10,48,38,26,15,15,4,26,31,79,
  78,0,31,79,70,70,0,2,1,39,52,14,8,2,26,31,31,79,78,0,
  79,70,70,0,67,5,8,3,46,10,2,26,51 };
  
// this structure defines all the variables and events of your control interface 
struct {

    // input variables
  uint8_t btn_preprod_onoff; // =1 if state is ON, else =0 
  uint8_t sw_preprod_pulse; // =1 if switch ON and =0 if OFF 
  uint8_t btn_prod_onoff; // =1 if state is ON, else =0 
  uint8_t sw_prod_pulse; // =1 if switch ON and =0 if OFF 

    // output variables
  char text_1[51];  // string UTF8 end zero 

    // other variable
  uint8_t connect_flag;  // =1 if wire connected, else =0 

} RemoteXY;
#pragma pack(pop)
/////////////////////////////////////////////
//           END RemoteXY include          //
/////////////////////////////////////////////


// LED connected to GPIO 2
#define ledPin1 2

#define relay_preprod_pin 4
#define relay_prod_pin 5

// ESP8266 Analog Pin ADC0 = A0
#define analogPin A0 

bool serialSetupDone;
Async_Operations * clock_tick;


void setupIO()
{
  pinMode(relay_preprod_pin, OUTPUT);
  pinMode(relay_prod_pin, OUTPUT);
}


void setupGlobals(){
  //clock_tick = new Async_Operations(1000LL);
  serialSetupDone = false;
}

void setupSerial()
{
  if (serialSetupDone) return;
  Serial.begin(115200);
  Serial.println();
  serialSetupDone = true;
}


void everySecond() {
  const char * msg = RemoteXY_isConnected()?"Still connected":"Not connected";
  if (! serialSetupDone) return;
  Serial.println(msg);
}


void setup() 
{
  setupGlobals();
  setupIO();
  setupSerial();
  //clock_tick->setLoopCallback(&everySecond);
  //clock_tick->start();
  //esp_delay(5000);
  Serial.println("Init done: Connecting....");
  //remotexy = new CRemoteXY (RemoteXY_CONF_PROGMEM, &RemoteXY, "", new CRemoteXYConnectionCloud (new CRemoteXYComm_WiFi ("Hydra", "K5x48Vz3"), "cloud.remotexy.com", 6376, "0d15d9a763d644975b4789cf2f8d9ce0"))
  RemoteXY_Init(); 
  
  Serial.println("Connected?");
  if(RemoteXY_isConnected()) Serial.println("Yes");
  else Serial.println("NO!");
  // TODO you setup code
  updateMsg(L"Hello");
}

void handlePreprodButton(evt_time_t, evt_data_t) {
  digitalWrite(relay_preprod_pin, RemoteXY.btn_preprod_onoff);
  //if(RemoteXY.sw_preprod_pulse)triggerFuture();
}

void handleProdButton(evt_time_t, evt_data_t) {
  digitalWrite(relay_prod_pin, RemoteXY.btn_prod_onoff);
  //if(RemoteXY.sw_prod_pulse)triggerFuture();
}

void handlePreprodSw(evt_time_t, evt_data_t) {

}

void handleProdSw(evt_time_t, evt_data_t) {

}

typedef std::basic_string<wchar_t> wstring;

void updateMsg(const wchar_t * const s)
{
  swprintf((wchar_t *)RemoteXY.text_1, (sizeof(RemoteXY.text_1)/2)-1,L"%25s", s);
}

EventDetect preprod_btn_evt(&RemoteXY.btn_preprod_onoff, handlePreprodButton);
EventDetect prod_btn_evt(&RemoteXY.btn_prod_onoff, handleProdButton);
EventDetect preprod_sw_evt(&RemoteXY.sw_preprod_pulse, handlePreprodSw);
EventDetect prod_sw_evt(&RemoteXY.sw_preprod_pulse, handleProdSw);


void loop() 
{ 
  RemoteXY_Handler();
  if (! RemoteXY.connect_flag)
  {
    updateMsg(L"Connection Lost");
    return;
  }
  //clock_tick->update();
  /*
  preprod_btn_evt.update();
  prod_btn_evt.update();
  preprod_sw_evt.update();
  prod_sw_evt.update();
*/

}


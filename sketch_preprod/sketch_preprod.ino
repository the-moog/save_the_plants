#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "app.h"

// Defined in app.h
const char auth[] = BLYNK_AUTH_TOKEN;

// Defined in secrets.h (not included for obvious reasons,
// see secrets.tmplh for basis)
const char ssid[]   = WIFI_SSID;
const char password[] = WIFI_PASS;

const char * const days[] = {
  "Sunday", "Monday", "Tuesday", "Wednesday",
  "Thursday", "Friday", "Saturday"};
  
unsigned long upTime;
unsigned wateringDuration;

WiFiUDP ntpUDP;

BlynkTimer timer;

bool serialSetupDone;
bool blynkSetupDone;
bool pumpOn;
bool pumpOverride;
double adcValue;
double wetness;

// timeClient(connection,server address, offset in ms, update rate in ms);
NTPClient timeClient(ntpUDP,"europe.pool.ntp.org", 0, 10000);



bool wateringInProcess()
{
  return (wateringDuration > 0) || pumpOverride?true:false;
}

void setupWiFi()
{
  setupSerial();

  WiFi.begin(ssid, password);

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.subnetMask());
  Serial.println(WiFi.gatewayIP());
  Serial.println(WiFi.dnsIP());

  WiFi.printDiag(Serial);
}



void setupIO()
{
  pinMode(ledPin1, OUTPUT);
  pinMode(relayPin, OUTPUT);
}

void setupGlobals(){
  serialSetupDone = false;
  upTime = 0L;
  wateringDuration = 0;
  blynkSetupDone = false;
  pumpOn = false;
  pumpOverride = false;
  adcValue = 0.0;
}

void setupSerial()
{
  if(serialSetupDone)return;
  Serial.begin(115200);
  Serial.println();
  serialSetupDone = true;
}



#if USE_SCHEDULE == 1
void wateringSchedule()
{
  // If we are already watering, we can't start it again
  if(wateringInProcess())return;

  bool ntpAlive = timeClient.isTimeSet();

  // backup plan, once every 7 days
  if (! ntpAlive)
  {
    if (upTime % (60*60*24*7) == 0)wateringDuration = 120;
    return;
  }


  unsigned int day = timeClient.getDay();
  unsigned int hour = timeClient.getHours();
  unsigned int mins = timeClient.getMinutes();
  unsigned int secs = timeClient.getSeconds();

  if(day == 6 && hour == 18 && mins == 0 && secs < 30)wateringDuration = 120;
}
#endif 


void doWatering()
{
  #if USE_SCHEDULE == 1
  wateringSchedule();
  #endif
  
  if(wateringInProcess()){
    digitalWrite(relayPin, LOW);
    pumpOn = true;
  }
  else {
    digitalWrite(relayPin, HIGH);
    pumpOn = false;
  }
}

void doBlinkLED()
{
  digitalWrite(ledPin1, LOW);
  delay(100);
  digitalWrite(ledPin1, HIGH);
  timeClient.update();
  if(wateringInProcess() && wateringDuration > 0)wateringDuration--;
  upTime++;
}


void doReport()
{
  bool ntpAlive = timeClient.isTimeSet();
  if (ntpAlive or (upTime % 5 == 0)){
    if(timeClient.getEpochTime() % 5 == 0 || !ntpAlive)
    {
      unsigned int day = timeClient.getDay();
      Serial.printf("Today is %s", days[day]);

      Serial.printf(", the time is: %s", timeClient.getFormattedTime());

      
      Serial.printf(", Wetness: %6.2f", wetness);

      Serial.printf(", Watering is %s", wateringInProcess()?"ON":"OFF");
      if(wateringInProcess())Serial.printf(", %u seconds remaining\n", wateringDuration);
      else printf("\n");
    }
  }
}

// This function sends Arduino's uptime every second to Virtual Pin 2.
void myTimerEvent()
{
  adcValue = analogRead(analogPin) * 1.0;
  wetness = 102400/(3.4*adcValue);
  // You can send any value at any time.
  // Please don't send more that 10 values per second.
  if(blynkSetupDone){
    // V2 = upTime
    Blynk.virtualWrite(V2, upTime);

    // V4 = Pump Status
    Blynk.virtualWrite(V4, pumpOn);

    // V6 = Wetness sensor
    Blynk.virtualWrite(V6, wetness);
  }
  doBlinkLED();
  doReport();
}

void setupBlynk()
{
  //Blynk.begin(auth, ssid, pass);
  // You can also specify server:
  //Blynk.begin(auth, ssid, pass, "blynk.cloud", 80);
  //Blynk.begin(auth, ssid, pass, IPAddress(192,168,1,100), 8080);

  Serial.println("Connecting Blynk");
  // Setup Blynk
  Blynk.config(auth);
  while (Blynk.connect() == false) {
    Serial.print("+");
  }

  Serial.println("Blynk Connected");

  blynkSetupDone = true;
}

void setup()
{
  setupGlobals();
  setupIO();
  setupSerial();
  setupWiFi();
  timeClient.begin();

  // Setup a function to be called every second
  timer.setInterval(1000L, myTimerEvent);
  Serial.setDebugOutput(true);

  setupBlynk();
}

// The 'main' is always called 'loop'
void loop()
{
  if(blynkSetupDone)Blynk.run();
  timer.run();
  doWatering();
}

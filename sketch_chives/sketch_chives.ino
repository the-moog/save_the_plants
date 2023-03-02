#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

//#include <ESP8266Ping.h>

#define BLYNK_TEMPLATE_ID "TMPLBeqnfO66"
#define BLYNK_DEVICE_NAME "Chives"
#define BLYNK_AUTH_TOKEN BLYNK_TOKEN_4

// Comment this out to disable prints and save space
#define BLYNK_PRINT Serial
//#define BLYNK_DEBUG 

// NOTE This must be included after the BLYNK definitions
// otherwise it fails to connect
#include <BlynkSimpleEsp8266.h>

const char auth[] = BLYNK_AUTH_TOKEN;

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

// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
NTPClient timeClient(ntpUDP);

// You can specify the time server pool and the offset, (in seconds)
// additionally you can specify the update interval (in milliseconds).
// NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);




// LED connected to GPIO 2
#define ledPin1 2
// LED & relayconnected to GPIO 16
#define relayPin 16
// ESP8266 Analog Pin ADC0 = A0
#define analogPin A0 

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

  /*
  int cnt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (cnt++ >= 10) {
      WiFi.beginSmartConfig();
      while (1) {
        delay(1000);
        if (WiFi.smartConfigDone()) {
          Serial.println();
          Serial.println("SmartConfig: Success");
          break;
        }
        Serial.print("|");
      }
    }
  }
*/
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

void doWatering()
{
  wateringSchedule();
  
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
/*
  if(Ping.ping(remote_host)) {
    Serial.println("Success!!");
  } else {
    Serial.println("Error :(");
  }
*/
  
  setupBlynk();
}


void loop()
{
  if(blynkSetupDone)Blynk.run();
  timer.run();
  doWatering();
}

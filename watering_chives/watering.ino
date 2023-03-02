#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

//#include <ESP8266Ping.h>


const char ssid[]   = WIFI_SSID
const char password[] = WIFI_PASS;

const char * const days[] = {
  "Sunday", "Monday", "Tuesday", "Wednesday",
  "Thursday", "Friday", "Saturday"};
  
unsigned long upTime;
unsigned wateringDurationTimer;
unsigned wateringDurationSeconds;
unsigned wateringFrequencyHours;


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
  return (wateringDurationTimer > 0) || pumpOverride?true:false;
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
  wateringDurationTimer = 0;
  blynkSetupDone = false;
  pumpOn = false;
  pumpOverride = false;
  adcValue = 0.0;
  wateringDurationSeconds = 10;
  wateringFrequencyHours = 48;
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

  // backup plan, once every 7 days
  if (! timeClient.isTimeSet())
  {
    if ((upTime > wateringDurationSeconds) && 
      (upTime % (60*60*24*7) == 0))wateringDurationTimer = wateringDurationSeconds;
    return;
  }
  
  unsigned int day = timeClient.getDay();
  unsigned int hour = timeClient.getHours();
  unsigned int mins = timeClient.getMinutes();
  unsigned int secs = timeClient.getSeconds();

  // Schedules are timed from a Saturday night at midnight
  if(day == 6 && hour == 18 && mins == 0 && secs < 30)wateringDurationTimer = wateringDurationSeconds;
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
  timeClient.update();
  digitalWrite(ledPin1, LOW);
  delay(timeClient.isTimeSet()?50:200);
  digitalWrite(ledPin1, HIGH);
  if(wateringInProcess() && wateringDurationTimer > 0)wateringDurationTimer--;
  upTime++;
}


// This function is called every time the Virtual Pin 0 state changes
void blynk_write_V0(int value)
{
  // Just loop the value back
  Blynk.virtualWrite(V1, value);
}

// This function is called every time the Virtual Pin 5 changes state
void blynk_write_V5(int value)
{
  pumpOverride = value>0?true:false;

  if(!pumpOverride)wateringDurationTimer = 0;
}

// Watering duration in seconds
// This function is called every time the Virtual Pin 5 changes state
void blynk_write_V7(int value)
{
  wateringDurationSeconds = value;
}

// Watering frequency in hours
// This function is called every time the Virtual Pin 5 changes state
void blynk_write_V8(int value)
{
  wateringFrequencyHours = value;
}

// This function is called every time the device is connected to the Blynk.Cloud
void blynk_connected()
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
      if(wateringInProcess())Serial.printf(", %u seconds remaining\n", wateringDurationTimer);
      else printf("\n");
    }
  }
}


void myTimerEvent()
{
  adcValue = analogRead(analogPin) * 1.0;
  wetness = 102400/(3.4*adcValue);
  
  // Please don't send more that 10 values per second.
  if(blynkSetupDone){
    // V2 = upTime
    Blynk.virtualWrite(V2, upTime);

    // V4 = Pump Status
    Blynk.virtualWrite(V4, pumpOn);

    // V6 = Wetness sensor
    Blynk.virtualWrite(V6, wetness);

    // V6 = Raw ADC
    Blynk.virtualWrite(V9, adcValue);
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

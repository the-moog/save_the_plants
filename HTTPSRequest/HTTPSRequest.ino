/*
    HTTP over TLS (HTTPS) example sketch

    This example demonstrates how to use
    WiFiClientSecure class to access HTTPS API.
    We fetch and display the status of
    esp8266/Arduino project continuous integration
    build.

    Created by Ivan Grokhotkov, 2015.
    This example is in public domain.
*/

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include "certs.h"
#include "secrets.h"

#ifndef STASSID
#define STASSID WLAN_SSID
#define STAPSK WLAN_PASS 
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

X509List cert(cert_DigiCert_Global_Root_CA);

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Set time via NTP, as required for x.509 validation
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));

  // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;
  Serial.print("Connecting to ");
  Serial.println(github_host);

  Serial.printf("Using certificate: %s\n", cert_DigiCert_Global_Root_CA);
  client.setTrustAnchors(&cert);

  if (!client.connect(github_host, github_port)) {
    Serial.println("Connection failed");
    return;
  }

  String url = "/repos/esp8266/Arduino/commits/master/status";
  Serial.printf("Requesting URL: https://%s:%d%s\n",github_host,github_port,url.c_str());
  String req = String("GET ") + url + " HTTP/1.1\r\n" +
    "Host: " + github_host + "\r\n" +
    "Accept: text/html,application/xhtml+xml,application/xml;"\
    "q=0.9,image/avif,image/webp,*/*;q=0.8\r\n" +
    "User-Agent: ESP8266\r\n" + 
    "Pragma: no-cache\r\n" +
    "Cache-Control: no-cache\r\n" +
   // "Connection: close\r\n\r\n";
    "\r\n";
  Serial.printf("-> Req: %s\n", req.c_str());
  client.print(req.c_str());

  Serial.println("Request sent");
  Serial.print("<- Reply Header: ");
  unsigned long timeout = millis()+2000;
  unsigned retry = 5;
  size_t expect_body_len = 0;
  do {
    String line = client.readStringUntil('\n');
    Serial.println(line);
    line.trim();

    if (line == "") {
      //Empty line ends header
      Serial.println("Header received");
      break;
    }

    if (line.startsWith("Content-Length: ")) {
      Serial.println("Extract Len");
      expect_body_len = line.substring(line.indexOf(": ")+1).toInt();
    }

    if(line.length()) {
      retry = 5;
    } else if (millis() > timeout)retry --;

    if(millis() > timeout)timeout = millis() + 400;
  } while (retry);

  if(expect_body_len) Serial.printf("\r\nExpecting body of %lu bytes\n", expect_body_len);

  size_t body_len = 0;
  char buffer[513];
  retry = 5;

  if(client.peekAvailable() ) {
    timeout = millis()+2000;
    Serial.println("<- Reply Body:");

    do {
      size_t n_read = min<size_t>(client.peekAvailable(), 512);
      n_read = client.readBytes(buffer, n_read);
      if(n_read>=0) {
        buffer[n_read] = '\0';
        body_len += n_read;
      }

      if(strlen(buffer))Serial.printf("%s", buffer);

      if(body_len >= expect_body_len)break;

      if(n_read) {
        retry = 5;
      } else if(millis() > timeout) {
        retry --;
      }

      if(millis()>timeout)timeout = millis() + 400;
    } while (retry);
    Serial.printf("\r\nRead %lu bytes\n", body_len);
    if(body_len == expect_body_len)Serial.println("As expected");
  }
  

/*
  if (line.startsWith("{\"state\":\"success\"")) {
    Serial.println("esp8266/Arduino CI successful!");
  } else {
    Serial.println("esp8266/Arduino CI has failed");
  }*/
  Serial.println("\r\n\nClosing connection");
  client.stopAll();
}

void loop() {}

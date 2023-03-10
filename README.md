# save_the_plants
On holiday and did not want to kill my plants.  Here is a cheap remote IoT watering solution using Expressif ES8266


Components:
TODO

Circuit:
TODO


Deploying:

Create an account at https://blink.cloud
Adding a device will provide a template ID and device token



Build environment:

Install Arduino IDE (i used v.1.8.5)
Add the following line in preferences for additional board files
https://arduino.esp8266.com/stable/package_esp8266com_index.json

Use the Tools -> Board menu to select the "Generic ESP8266" Board

Prep:

Copy secrets.tmplh to secrets.h and provide
WIFI_SSID       Your wifi network SSID
WIFI_PASS       Your wifi network password
BLYNK_TOKEN_1   The Blynk supplied target token

Edit blynk_template.h
Ensure you provide a template ID that matches the value supplied by Blynk

Edit target_node.h
- Ensure the `BLYNK_DEVICE_NAME` is descriptive so if you have multiple
target nodes they all don't have the same name
- Ensure the `BLYNK_TEMPLATE_ID` matches the correct name from blynk_template.h, above.
Ensure `BLYNK_AUTH_TOKEN` matches the correct name from secrets.h

Building:
Press the tick on the menu bar

Deploying:
Ensure the CP2102 driver is installed from

Plug the board into a USB

Sketch -> Upload



References:

Arduino API         : https://www.arduino.cc/reference/en/
Blynk Firmware API  : https://docs.blynk.io/en/blynk.edgent-firmware-api
Turorial            : https://randomnerdtutorials.com/how-to-install-esp8266-board-arduino-ide/
Arduino API         : https://arduino-esp8266.readthedocs.io/en/latest/installing.html

https://www.az-delivery.de/en/products/nodemcu-lua-amica-v2-modul-mit-esp8266-12e-unverloetet
https://www.hackster.io/javagoza/nodemcu-amica-v2-road-test-2e8bff
https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers?tab=downloads
https://www.espressif.com/en/support/documents/technical-documents
https://www.electronicshub.org/esp8266-nodemcu-input-output/


IOPins
The writing on the PCB bares little resemblance to the IOPins in C


30  Vin (USB Power)
29  GND
28  RESET (Button)
27  Enable
26  3.3V
25  GND
24  SCLK/SDCLK  (Used for external flash)
23  MISO/SDD0
22  CS/SDCMD
21  MISO/SDD1
20  GPIO9/SDD2
19  GPIO10/SDD3
18  Reserved
17  Reserved
16  ADC0
15  3.3V
14  GND
13  GPIO1   TXD0
12  GPIO3   RXD0
11  GPIO15  TXD2/CS#
10  GPIO13  RXD2/MOSI
9   GPIO12  MISO
8   GPIO14  SCLK
7   GND
6   3.3V
5   GPIO2/TXD1
4   FPIO0/FLASH  (Button)
3   SPIO4/SDA
2   GPIO5/SCL
1   GPIO16/WAKE
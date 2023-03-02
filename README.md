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

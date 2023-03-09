set server=%1
set port=
if exist certs.h if exist secrets.h (
    FOR /F "usebackq tokens=2" %%i IN (`python %~dp0\h_parser.py -f -d MQTT_HOST secrets.h`) DO set server=-s %%i
    FOR /F "usebackq tokens=2" %%i IN (`python %~dp0\h_parser.py -f -d MQTT_PORT secrets.h`) DO set port=-p %%i
    )
python %~dp0\cert.py %server% %port% -n myhost > certs.h

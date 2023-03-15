import random
import sys
import time
from h_parser import get_c_defines
from cert import get_ca_pem
import random
import threading

from paho.mqtt import client as mqtt_client


#  Defined in secrets.h
defines = get_c_defines(("MQTT_HOST", "MQTT_PORT", "MQTT_USERNAME", "MQTT_KEY"), "secrets.h")
MQTT_KEY = defines['MQTT_KEY']
MQTT_USERNAME = defines['MQTT_USERNAME']
MQTT_BROKER = defines["MQTT_HOST"]
MQTT_PORT = defines["MQTT_PORT"]
CLIENT_ID = "devsys-mqtt-py-{id}".format(id=random.randint(0, 1000))

feeds = {
    "ONOFF0": f'dev/onoff0',        # /dev/onoff<ch> <0|1>  ch=0|1, 1=on 0=off
    "ONOFF1": f'dev/onoff1',
    "PULSE_MS": f'cfg/pulse_ms',    # Pulse duration in milliseconds (float)
    "PULSE0": f'dev/pulse0',        # /dev/pulse<ch>   ch=0|1 - Invert on/off state for cfg/pulse_ms
    "PULSE1": f'dev/pulse1'
}


class RemoteRelays:
    
    def __init__(self):
        self.client = mqtt_client.Client(CLIENT_ID, transport='websockets')
        self.thread = threading.Thread(target=self.client.loop_forever())
        self.thread.start()

    def on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            print(f"Connected to MQTT Broker {MQTT_BROKER}!")
            for feed, path in feeds.items():
                print(f"Subscribing to {feed} on {path}")
                self.client.subscribe(path)
        else:
            print(f"Failed to connect to {MQTT_BROKER}, return code {rc}".format(rc=rc), )

    def disconnect(self):
        print('Disconnecting from host!')
        self.client.disconnect()
        self.client.loop_stop()

    def on_message(self, client, userdata, msg):
        print("Received `{payload}` from `{topic}` topic".format(
            payload=msg.payload.decode(), topic=msg.topic))

    def power_onoff(self, channel, onoff):
        self.client._client.publish(feeds[f"ONOFF{channel}"], onoff)

    def set_pulse_ms(self, ms):
        self.client._client.publish(feeds[f"PULSE_MS"], f"{ms}")

    def power_pulse(self, channel, ms=None):
        if ms is not None:
            self.set_pulse_ms(ms)
        self.client._client.publish(feeds["PULSE{channel}"])

    def connect(self):
        
        #cert = get_ca_pem(MQTT_BROKER, MQTT_PORT)
        #ctx = ssl.context(cert)
        #client.tls_set_context(ctx)
        #Mandate TLS1.2
        self.client.tls_set()
        self.client.username_pw_set(MQTT_USERNAME, MQTT_KEY)
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.connect(MQTT_BROKER, MQTT_PORT)
        return self.client


rr = RemoteRelays()


class TestPattern(threading.Thread):
    def __init__(self):
        self.evt = threading.Event()
        super().__init__()
        self.start()

    def run(self):
        while True:
            if rr.client.is_connected:
                channel = random.choice([0,1])
                value = random.choice([0,1])
                print(f'Publishing {value} to onoff{channel}')
                rr.power_onoff(channel, value)
            if self.evt.is_set():
                break
            time.sleep(5)





if __name__ == '__main__':
    tp = TestPattern()
    rr.connect()

    while True:
        try:
            time.sleep(10)
        except InterruptedError:
            tp.evt.set()
            break
    tp.join()
    rr.disconnect()
    
        

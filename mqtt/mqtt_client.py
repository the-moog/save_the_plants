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


feeds = {                           # feed              msg     - Notes:
    "ONOFF": f'dev/onoff/#',        # /dev/onoff/<ch>   <0|1>   - Turn on or off, ch=0|1, 1=on 0=off
    "PULSE_MS": f'cfg/pulse_s',     # /cfg/pulse_s      <s>     - Pulse duration in seconds (float) >= 1ms
    "PULSE": f'dev/pulse/#',        # /dev/pulse/<ch>   n/a     - Invert on/off state for cfg/pulse_ms
}


def random_client_id():
    return "devsys-mqtt-py-{id}".format(id=random.randint(0, 1000))

class RemoteRelays:
    
    def __init__(self, client_id=None):
        if client_id is None:
            client_id = random_client_id()

        self.client = mqtt_client.Client(client_id, transport='websockets')
        self.thread = threading.Thread(target=self.client.loop_forever)
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

    @property
    def is_connected(self):
        return self.client.is_connected()

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


class TestPattern(threading.Thread):
    def __init__(self):
        self.rr = RemoteRelays()
        self.evt = threading.Event()
        super().__init__()
        self.start()

    def run(self):
        self.rr.connect()
        while True:
            if self.rr.is_connected:
                channel = random.choice([0,1])
                value = random.choice([0,1])
                print(f'Publishing {value} to onoff{channel}')
                self.rr.power_onoff(channel, value)
            if self.evt.is_set():
                break
            time.sleep(5)

        self.rr.disconnect()


class OccasionalPulse(threading.Thread):
    def __init__(self):
        self.rr = RemoteRelays()
        self.evt = threading.Event()
        super().__init__()
        self.last_time = None
        self.next_time = None
        self.start()
        
    @property
    def next_pulse(self):
        if self.last_time is None or time.time() > self.last_time:
            now = time.time()
            self.last_time = now
            self.next_time = now + random.randint(1*60, 2*60)
            return True
        return False

    def run(self):
        self.rr.connect()
        while True:
            if self.rr.is_connected and self.next_pulse:
                channel = random.choice([0,1])
                print(f'Publishing pulse to channel {channel}')
                self.rr.power_pulse(channel)
            if self.evt.is_set():
                break
            time.sleep(0.1)
        self.rr.disconnect()



if __name__ == '__main__':
    tp = TestPattern()
    op = OccasionalPulse()

    print("Press ctrl-c to stop")
    while True:
        try:
            time.sleep(10)
        except InterruptedError:
            tp.evt.set()
            op.evt.set()
            break
    tp.join()
    op.join()


        

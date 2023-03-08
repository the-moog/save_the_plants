import random
import sys
import time
from h_parser import get_c_defines
from Adafruit_IO import MQTTClient


#  Defined in secrets.h
defines = get_c_defines(("AIO_KEY", "AIO_USERNAME"), "secrets.h")
AIO_KEY = defines['AIO_KEY']
AIO_USERNAME = defines['AIO_USERNAME']

feeds = {
    "SECONDS":f'time/seconds', 
    "ONOFF0": f'{AIO_USERNAME}/feeds/onoff0',
    "ONOFF1": f'{AIO_USERNAME}/feeds/onoff1',
    "PULSE_MS": f'{AIO_USERNAME}/feeds/pulse_ms',
    "PULSE0": f'{AIO_USERNAME}/feeds/pulse0',
    "PULSE1": f'{AIO_USERNAME}/feeds/pulse1'
}

def connected(client):
    print('Connected to Adafruit IO!  Subscribing')
    for feed, path in feeds.items():
        print(f"Subscribing to {feed} on {path}")
        client.subscribe(path)

def disconnected(client):
    # Disconnected function will be called when the client disconnects.
    print('Disconnected from Adafruit IO!')
    sys.exit(1)

def message(client, feed_id, payload):
    # Message function will be called when a subscribed feed has a new value.
    # The feed_id parameter identifies the feed, and the payload parameter has
    # the new value.
    print('Feed {0} received new value: {1}'.format(feed_id, payload))


def power_onoff(channel, onoff):
    client._client.publish(feeds[f"ONOFF{channel}"], onoff)

def set_pulse_ms(ms):
    client._client.publish(feeds[f"PULSE_MS"], f"{ms}")

def power_pulse(channel, ms=None):
    if ms is not None:
        set_pulse_ms(ms)
    client._client.publish(feeds["PULSE{channel}"])


# Create an MQTT client instance.
client = MQTTClient(AIO_USERNAME, AIO_KEY)

# Setup the callback functions defined above.
client.on_connect    = connected
client.on_disconnect = disconnected
client.on_message    = message

# Connect to the Adafruit IO server.
client.connect()

client.loop_background()


while True:
    channel = random.choice([0,1])
    value = random.choice([0,1])
    print(f'Publishing {value} to onoff{channel}')
    power_onoff(channel, value)
    time.sleep(5)



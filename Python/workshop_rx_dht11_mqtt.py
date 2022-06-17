import sys
import argparse
import time
import struct
import paho.mqtt.client as mqtt
from json import dumps
from RF24 import RF24, RF24_PA_LOW, RF24_250KBPS
from datetime import datetime as dt

# MQTT configuration
broker = "demo.thingsboard.io"
port = 1883
topic = 'v1/devices/me/telemetry'
access_token = 'vuUIAoUjTbJNRw0Fii2r'

# nrf24 configuration
radio = RF24(17, 0)
node_addresses = [
    b"\x78" * 5,
    b"\xF1\xB6\xB5\xB4\xB3",
    b"\xCD\xB6\xB5\xB4\xB3",
    b"\xA3\xB6\xB5\xB4\xB3",
    b"\x0F\xB6\xB5\xB4\xB3",
    b"\x05\xB6\xB5\xB4\xB3"
]

def nrf_receive(timeout=10):
    # write the addresses to all pipes.
    for pipe_n, addr in enumerate(node_addresses):
        radio.openReadingPipe(pipe_n, addr)
    radio.startListening()  # put radio in RX mode
    start_timer = time.monotonic()  # start timer
    while time.monotonic() - start_timer < timeout:
        has_payload, pipe_number = radio.available_pipe()
        if has_payload:
            # unpack payload
            nodeID, payloadID, humidity, temperature = struct.unpack(
                "<IIff",
                radio.read(radio.payloadSize)
            )
            # show the pipe number that received the payload
            print(
                "Received {} bytes on pipe {} from node {}. PayloadID: {} "
                "Humidity: {} Temperature: {}".format(
                    radio.payloadSize,
                    pipe_number,
                    nodeID,
                    payloadID,
                    humidity,
                    temperature
                )
            )
            start_timer = time.monotonic()  # reset timer with every payload
            radio.stopListening()
            return {'nodeID': nodeID, 'humidity': humidity, 'temperature': temperature}

    print("Nothing received in", timeout, "seconds. Leaving RX role")
    radio.stopListening()
    return None


if __name__ == "__main__":

    # setup MQTT connection
    client1 = mqtt.Client("pub01")
    client1.username_pw_set(username=access_token)
    client1.connect(broker, port)

    # initialize the nRF24L01 on the spi bus
    if not radio.begin():
        raise RuntimeError("radio hardware is not responding")

    # radio.setPALevel(RF24_PA_LOW)  # RF24_PA_MAX is default
    radio.setDataRate(RF24_250KBPS)
    radio.setChannel(100)
    radio.payloadSize = 5    

    # 2 int occupy 8 bytes in memory using len(struct.pack())
    # "<II" means 2x little endian unsigned int
    radio.payloadSize = len(struct.pack("<IIff", 0, 0, 0.0, 0.0))

    while True:
        try:
            resp = nrf_receive()
            if resp is not None:
                resp['timestamp'] = dt.now().strftime('%Y %b %d, %H:%M:%S')
                ret = client1.publish(topic, dumps(resp))
        except KeyboardInterrupt:
            print(" Keyboard Interrupt detected. Exiting...")
            client1.disconnect()
            radio.powerDown()
            sys.exit()

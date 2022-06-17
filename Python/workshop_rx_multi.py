import sys
import time
import struct
from RF24 import RF24, RF24_PA_LOW, RF24_250KBPS


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
            sensorID, *unTemp = struct.unpack(
                "c4c", radio.read(radio.payloadSize)
            )
            unTemp = ''.join([b.decode() for b in unTemp])
            # show the pipe number that received the payload
            print(
                f"Received {radio.payloadSize} bytes on pipe {pipe_number} from sensor {sensorID}: {unTemp}"
            )
            start_timer = time.monotonic()  # reset timer with every payload
            radio.stopListening()
            return {'sensorID': sensorID.decode(), 'unTemp': int(unTemp)}

    print("Nothing received in", timeout, "seconds. Leaving RX role")
    radio.stopListening()
    return None


if __name__ == "__main__":

    # initialize the nRF24L01 on the spi bus
    if not radio.begin():
        raise RuntimeError("radio hardware is not responding")

    # radio.setPALevel(RF24_PA_LOW)  # RF24_PA_MAX is default
    radio.setDataRate(RF24_250KBPS)
    radio.setChannel(100)
    radio.payloadSize = 5

    while True:
        try:
            resp = nrf_receive()
            if resp is not None:
                print(resp)
        except KeyboardInterrupt:
            print(" Keyboard Interrupt detected. Exiting...")
            radio.powerDown()
            sys.exit()

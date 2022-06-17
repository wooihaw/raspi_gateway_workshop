#include <SPI.h>
#include "RF24.h"

// instantiate an object for the nRF24L01 transceiver
RF24 radio(7, 8); // using pin 7 for the CE pin, and pin 8 for the CSN pin

int nodeID = 2;  // change nodeID (from 0 to 5)

uint64_t address[6] = {0x7878787878LL,
                       0xB3B4B5B6F1LL,
                       0xB3B4B5B6CDLL,
                       0xB3B4B5B6A3LL,
                       0xB3B4B5B60FLL,
                       0xB3B4B5B605LL
                      };

unsigned long last_reading;                // Milliseconds since last measurement was read.
unsigned long ms_between_reads = 5000;    // 5000 ms = 5 seconds

char payload[5] = "a1234";

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    // some boards need to wait to ensure access to serial over USB
  }

  // initialize the transceiver on the SPI bus
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {} // hold in infinite loop
  }

  // print example's introductory prompt
  Serial.println(F("NRF24L01 initialized"));

//  radio.setPALevel(RF24_PA_LOW); // RF24_PA_MAX is default.
  radio.setChannel(100);
  radio.setDataRate(RF24_250KBPS);
  radio.setPayloadSize(5);

  // Set the address on pipe 0 to the RX node.
  radio.stopListening(); // put radio in TX mode
  radio.openWritingPipe(address[nodeID]);

  // According to the datasheet, the auto-retry features's delay value should
  // be "skewed" to allow the RX node to receive 1 transmission at a time.
  // So, use varying delay between retry attempts and 15 (at most) retry attempts
  radio.setRetries(((nodeID * 3) % 12) + 3, 15); // maximum value is 15 for both args
}

void loop() {
  if (millis() - last_reading > ms_between_reads) {
  
    // This device is a TX node
    bool report = radio.write(&payload, sizeof(payload));    // transmit & save the report

    if (report) {
      // payload was delivered
  
      Serial.print(F("Transmission as "));
      Serial.print(nodeID);                          // print nodeID
      Serial.print(F(" successful!"));
      Serial.print(F(" Payload size: "));
      Serial.print(sizeof(payload));
    } else {
      Serial.println(F("Transmission failed or timed out")); // payload was not delivered
    }

    last_reading = millis();
  }
}

#include <SPI.h>
#include "RF24.h"

#include <DHT.h>

#define PIN_DHT                  2            // PIN for DHT sensor communication.
#define DHT_TYPE              DHT11           // Type of DHT sensor:
                                              // DHT11, DHT12, DHT21, DHT22 (AM2302), AM2301

#define DEBUG 1

// Create DHT sensor
DHT dht(PIN_DHT, DHT_TYPE);

unsigned long last_reading;                // Milliseconds since last measurement was read.
unsigned long ms_between_reads = 5000;    // 5000 ms = 5 seconds

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

struct PayloadStruct
{
  unsigned long nodeID;
  unsigned long payloadID;
  float humidity;
  float temperature;
};
PayloadStruct payload;

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

  radio.setPayloadSize(sizeof(payload)); // 2x int  + 2x float datatype occupy 16 bytes

  // set the payload's nodeID & reset the payload's identifying number
  payload.nodeID = nodeID;
  payload.payloadID = 0;

  // Set the address on pipe 0 to the RX node.
  radio.stopListening(); // put radio in TX mode
  radio.openWritingPipe(address[nodeID]);

  // According to the datasheet, the auto-retry features's delay value should
  // be "skewed" to allow the RX node to receive 1 transmission at a time.
  // So, use varying delay between retry attempts and 15 (at most) retry attempts
  radio.setRetries(((nodeID * 3) % 12) + 3, 15); // maximum value is 15 for both args

  // Initialise the DHT sensor.
  dht.begin();

  // Take the current timestamp. This means that the next (first) measurement will be read and
  // transmitted in "ms_between_reads" milliseconds.
  last_reading = 0;
}

void loop() {
  if (millis() - last_reading > ms_between_reads) {
    // Read sensor values every "ms_between_read" milliseconds.
  
    // Read the humidity and temperature.
    float t, h;
    h = dht.readHumidity();
    t = dht.readTemperature();
      
    // This device is a TX node
    payload.humidity = h;
    payload.temperature = t;
    bool report = radio.write(&payload, sizeof(payload));    // transmit & save the report

    if (report) {
      // payload was delivered
  
      Serial.print(F("Transmission of payloadID "));
      Serial.print(payload.payloadID);                       // print payloadID
      Serial.print(F(" as node "));
      Serial.print(payload.nodeID);                          // print nodeID
      Serial.print(F(" successful!"));
      Serial.print(sizeof(payload));
      Serial.print(F(" Humidity: "));
      Serial.print(payload.humidity);                 // print the humidity
      Serial.print(F(" Temperature: "));
      Serial.println(payload.temperature);                 // print the temperature    
    } else {
      Serial.println(F("Transmission failed or timed out")); // payload was not delivered
    }
    payload.payloadID++;                                     // increment payload number
    last_reading = millis();
  }
}

#include <SPI.h>
#include "RF24.h"

#define PDEBUG_LED 6

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

#define _SENSOR_ID       'A'
char  strTX[16];          // TX string buffer

void setup() {
  pinMode(PDEBUG_LED, OUTPUT);
  
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

  // Set the address of the pipe. 
  // According to the datasheet, the auto-retry features's delay value should
  // be "skewed" to allow the RX node to receive 1 transmission at a time.
  // So, use varying delay between retry attempts and 15 (at most) retry attempts.
  #define   _DATAPIPE_NUMBER  2     // Valid value: 0 to 5
  
  #if  _DATAPIPE_NUMBER == 0
    radio.openWritingPipe(address[0]);
    radio.setRetries(0, 15);          // Delay 0 interval, max 15 retries.
  #elif _DATAPIPE_NUMBER == 1
    radio.openWritingPipe(address[1]);
    radio.setRetries(2, 15);          // Delay 2 intervals, max 15 retries.
  #elif _DATAPIPE_NUMBER == 2
    radio.openWritingPipe(address[2]);
    radio.setRetries(4, 15);          // Delay 4 intervals, max 15 retries.
  #elif _DATAPIPE_NUMBER == 3
    radio.openWritingPipe(address[3]);
    radio.setRetries(6, 15);          // Delay 6 intervals, max 15 retries.
  #elif _DATAPIPE_NUMBER == 4
    radio.openWritingPipe(address[4]);
    radio.setRetries(8, 15);          // Delay 8 intervals, max 15 retries.
  #else _DATAPIPE_NUMBER == 5
    radio.openWritingPipe(address[5]);
    radio.setRetries(10, 15);          // Delay 10 intervals, max 15 retries.
  #endif
}

void loop() {
  if (millis() - last_reading > ms_between_reads) {

    //Send message to receiver
    unsigned int unSensor;
    unsigned int unDigit;
    unsigned int unTen;
    unsigned int unHundred;
    unsigned int unThousand;
    unsigned int unTemp;
    unsigned int unSensor_mV;

    digitalWrite(PDEBUG_LED, HIGH);     // Turn on debug LED.
    unSensor = analogRead(2);    // Read value of analog sensor.
                                 // NOTE: The built-in ADC in Arduino Uno, Nano and
                                 // Micro and Pro-micro are 10 bits. With reference
                                 // voltage at 5V, each unit interval is 
                                 // 5/1023 = 4.888mV.                           
    unSensor_mV = (unSensor*49)/10;     // We use this trick so that the conversion
                                        // works entirely in integer, and within the
                                        // 16-bits limit of the integer datatype used
                                        // by Arduino.   
    unTemp = unSensor_mV;
  
    // --- Convert unsigned integer value from analog IR sensor to BCD ---
    // Format:
    // [Device ID]+[Thousand]+[Hundred]+[Ten]+[Digit]
    unDigit = 0;
    unTen = 0;
    unHundred = 0;
    unThousand = 0;
  
    while (unTemp > 999) // Update thousand.
    {
      unTemp = unTemp - 1000;
      unThousand++;
    }
    while (unTemp > 99)  // Update hundred.
    {
      unTemp = unTemp - 100;
      unHundred++;
    }
    while (unTemp > 9)  // Update ten.
    {
      unTemp = unTemp - 10;
      unTen++;
    }
    unDigit = unTemp;   // Update digit.
    // ---
    strTX[0] = _SENSOR_ID;          // Sensor ID
    strTX[1] = unThousand + 0x30;   // Convert unThousand to ASCII.
    strTX[2] = unHundred + 0x30;
    strTX[3] = unTen + 0x30;
    strTX[4] = unDigit + 0x30;
  
    // This device is a TX node
    bool report = radio.write(&strTX, 5);    // transmit & save the report

    if (report) {
      // payload was delivered
      digitalWrite(PDEBUG_LED, LOW);     // Turn on debug LED.
      Serial.print(F("Transmission as "));
      Serial.print(nodeID);                          // print nodeID
      Serial.print(F(" successful!"));
      Serial.print(F(" Payload: "));
      Serial.print(strTX);
    } else {
      Serial.println(F("Transmission failed or timed out")); // payload was not delivered
    }

    last_reading = millis();
  }
}

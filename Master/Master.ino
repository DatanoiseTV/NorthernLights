#include <SPI.h>
#include "nRF24L01.h" // library: https://github.com/maniacbug/RF24
#include "RF24.h"
#include <Wire.h>
#include <DMXSerial.h> // library: http://www.mathertel.de/Arduino/DMXSerial.aspx //

#define MAXGROUPS 17 // 17 groups of 30 channels = 510 channels
#define MAXPAYLOAD 32 // max payload size for nrf24l01
#define BURSTTIMER 40 // 40ms between blasts of radio data
#define DMX_LED 8 // DMX monitor : LED from digital pin 8 to 0V via 470R resistor

RF24 radio(9,10);

const uint64_t pipe = 0xF0F0F0F0E1LL; //0xE8E8F0F0E1LL;
uint8_t payload[MAXPAYLOAD];
unsigned long timeslot, flashTimer;
uint8_t timeStamp, channel=0; // ensure TX & RX channels are the same

void setup(void)
{
  pinMode(DMX_LED, OUTPUT);
  digitalWrite(DMX_LED, LOW);
  DMXSerial.init(DMXReceiver);
  radio.begin();
  radio.setAutoAck(false);
  radio.setPayloadSize(MAXPAYLOAD);
  radio.setPALevel(RF24_PA_HIGH);   
  radio.setDataRate(RF24_250KBPS);
  radio.openWritingPipe(pipe);
  radio.stopListening();
  radio.setChannel(channel);
  flashTimer = millis();
}

void loop(void)
{
  if (millis() - timeslot > BURSTTIMER) {
    timeslot = millis();
    for (uint8_t group = 0; group<=MAXGROUPS; group++) {
      payload[0] = group; // set first byte to point to group (groups of 30 bytes)
      payload[1] = timeStamp++; // second byte helps us monitor consistency of reception at receiver
      for (uint8_t chan = 1; chan<31; chan++) {
        payload[1+chan] = DMXSerial.read((group*30)+chan); // fill payload with DMX data
      }
      radio.write( payload, sizeof(payload) ); // dump payload to radio
      delayMicroseconds(20); // short delay between packets to ensure radio not overloaded   
    }
  }
  unsigned long lastPacket = DMXSerial.noDataSince();
  unsigned long lastFlash = millis() - flashTimer;
  if (lastPacket < 5000) { // if continuous DMX data is received then flash LED
    if (lastFlash < 500) { digitalWrite(DMX_LED,1); } // flash on 0.5sec
    else if (lastFlash < 1000) { digitalWrite(DMX_LED,0); } // flash off 0.5sec
    else if (lastFlash > 1000) { flashTimer = millis(); } // reset timer after 1 second
  }
}

// Arduino-based 3b1 mouse tester
// Prints data received from 3b1 mouse
// Written by Agent Booth 
#include <SoftwareSerial.h>

#define rxPin 2
#define txPin 3
#define resetPin 8

// if you want to see the hex data for the 3-byte packets
#define PRINT_RAW_PACKET

// need to use Software Serial as it supports inverse logic for the comm
SoftwareSerial softSerial(rxPin,txPin,true);  // Rx, Tx, inverse_logic=true!

void setup() {
  Serial.begin(115200);
  while(!Serial);
  Serial.println("debugging started");
  pinMode(resetPin, INPUT_PULLUP);  // connected to mouse Reset- pin, need to drive this high so no reset
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  softSerial.begin(1200);
  // mouse spits out a bunch of 0xFE's initially, and then two bytes (version number?)
  unsigned char ch = 0xFE;
  do {
    if (softSerial.available())
      ch = softSerial.read();
  } while (ch == 0xFE);
  Serial.print("version (?): ");
  Serial.print(ch, DEC);
  Serial.print("-");
  // need to read one more byte before the packets start
  while (!softSerial.available());
  Serial.println(softSerial.read(), DEC);
}

void loop() {
  // print data coming from mouse
  if (softSerial.available() >= 3)
  {
      unsigned char ch[3];
      ch[0] = softSerial.read();  // b4:xsign, b3:ysign, b2:L, b1:M, b0:R
      ch[1] = softSerial.read();  // b0-b6: x, b7: 0
      ch[2] = softSerial.read();  // b0-b6: y, b7: 1
#ifdef PRINT_RAW_PACKET
      Serial.print("packet: 0x");
      Serial.print(ch[0], HEX);
      Serial.print(" 0x");
      Serial.print(ch[1], HEX);
      Serial.print(" 0x");
      Serial.println(ch[2], HEX);
#endif
      if (bitRead(ch[0], 0)) Serial.println("RIGHT");
      if (bitRead(ch[0], 1)) Serial.println("MIDDLE");
      if (bitRead(ch[0], 2)) Serial.println("LEFT");
      
      Serial.print("X: ");
      if (!bitRead(ch[0], 4)) Serial.print("-"); 
      Serial.println(ch[1], DEC);
      
      Serial.print("Y: ");
      if (!bitRead(ch[0], 3)) Serial.print("-");
      Serial.println(ch[2] & 0x7F, DEC);
      
      Serial.println();
  }
}

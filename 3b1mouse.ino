// Arduino-based PS/2 mouse adapter to AT&T UNIX PC
// Written by Jesse Booth 
#include <SoftwareSerial.h>
#include "PS2Mouse.h"

#define RxPin 2
#define TxPin 3

// start of mouse packet, injected by logic built into keyboard
#define BEGMOUSE 0xCE
// start of keyboard packet
#define BEGKBD 0xDF
// keyboard code when all keys are up
#define KEY_ALL_UP 0x40
// keyboard code for end of list of keys pressed
#define KEY_LIST_END 0x80

// enable debugging to the USB serial port on pins 0 and 1
//#define DEBUG_TO_USB

// PS/2 pin 1 (data) -> Arduino pin 6
// PS/2 pin 5 (clock) -> Arduino pin 5
PS2Mouse mouse(5,6);  // clk, data

// need to use Software Serial as it supports inverse logic for the comm
SoftwareSerial softSerial(RxPin,TxPin,true);  // Rx, Tx, inverse_logic=true!

void setup() {
#ifdef DEBUG_TO_USB
  Serial.begin(115200);
  while(!Serial);
  Serial.println("debugging started");
#endif
  pinMode(RxPin, INPUT);
  pinMode(TxPin, OUTPUT);
  softSerial.begin(1200);
 
  mouse.begin();
}

void loop() {
  uint8_t stat, unixpc[3];
  int x,y;
  static uint8_t lastbut=0xFF;

  // stat: b5:ysign, b4:xsign, b2:M, b1:R, b0:L
  mouse.getPosition(stat,x,y);
  
  // unixpc[0]: b4:xsign, b3:ysign, b2:L, b1:M, b0:R
  unixpc[0] = bitRead(stat,0)<<2 | bitRead(stat,1) | bitRead(stat,2)<<1;
  if (x > 0) bitSet(unixpc[0],4);
  if (y > 0) bitSet(unixpc[0],3); 
  unixpc[1] = (abs(x) > 127) ? 127 : abs(x);
  unixpc[2] = (abs(y) > 127) ? 127 : abs(y);
  unixpc[2] |= 0x80;

  if (abs(x)!=0 || abs(y)!=0 || (unixpc[0] & 7)!=lastbut)
  {
    lastbut = unixpc[0] & 7;

    //transition to KMOUSE state, logic in keyboard is injecting this
    //softSerial.write(BEGMOUSE);
    softSerial.write(unixpc[0]);
    softSerial.write(unixpc[1]);
    softSerial.write(unixpc[2]);

    // need to double send BEGKBD (0xDF) if it's ever encountered
    // only possible in 3rd byte as high bit is never set in byte 0 or byte 1
    // logic in keyboard should take care of this
    //if (unixpc[2]==BEGKBD) softSerial.write(BEGKBD);

    //untested - this may work if you connect softSerial TxPin directly to the Rx pin on the motherboard port
    //(the port that the keyboard cable plugs into)
#ifdef KB_TEST
    if ((unixpc[0] & 5) == 5) // L&R buttons pressed
    {
      softSerial.write(BEGKBD);
      softSerial.write(0x67|KEY_LIST_END);  // send a 'g' char as a test with KEY_LIST_END
      softSerial.write(KEY_ALL_UP);
      Serial.println("g sent");
    }
#endif

    // transition to KQUOTE state, next BEGMOUSE will return to KMOUSE state
    // anything else will be treated as kbd char and transition back to KNORM (kbd) state
    // don't need to send it back to kbd state, logic in keyboard takes care of this
    //softSerial.write(BEGKBD);

#ifdef DEBUG_TO_USB
    // debug printing
    Serial.print("B0: ");
    Serial.print(unixpc[0], BIN);
    Serial.print(" B1: ");
    Serial.print(unixpc[1], DEC);
    Serial.print(" B2: ");
    Serial.print(unixpc[2] & 0x7F, DEC);
    Serial.println();
#endif
  }

  // debug incoming commands from CPU (e.g. CAPS LOCK LED ON/OFF)
  // TxPin on the mouse port is NOT connected to the mouse so this likely will not work
  // did not work for me - may work if you connect softSerial RxPin to Tx Pin on the motherboard port
  //if (softSerial.available()) Serial.println(softSerial.read(), HEX);

  delay(25);  // 40 Hz (don't exceed 1200 baud = roughly 3 byte mouse packets @ 40 Hz)
}

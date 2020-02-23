/* Panel0--primary processor for the knobs and buttons on the CNC mill at Maker Nexus.
 * Copyright (C) Dan Clemmensen, 2019. All rights reserved. Licensed to you 
 *  under the GPLv3 license.
 * control panel controller comprises two Arduino Nanos, Ar0 (this proicessor) and Ar1.
 * The panel has a bunch of dials and buttons. The Arduinos read them. Ar0 uses I2C to query 
 * Ar1, and Ar0 then includes all of its own Ar0 values and sends all values  
 * to the BBB control computer on the serial-over-USB interface.
 * We need Ar1 because
 * we have too many inputs for a single Nano and a second Nano is the most
 * cost-effective way to add I/O: it's cheaper than any other mux device.  We fill
 * up the pins on AR1 first because Ar0 will be doing the
 * extra work of listening to Ar1. Ar1 also handles the encoder wheels because it
 * requires extra processing. Ar0 and Ar1 communicate via I2C. AR0 is the I2C
 * master.
 * 
 * The AR0 loop sends an I2C master poll, waits for a response, moves the response
 * into the message for the BBB, moves its own button info into the message, and
 * sends the message to the BBB.
 */
#include <Wire.h>     //I2C library. uses HW I2C on pins A4 and A5
//we do not include softserial because we are using the HW serial port on D0 and D1.

//only nine button pins on Ar0
#define B_SPINDLE 2
#define B_FEEDRATE 3
#define B_JOGRATE 4
#define B_JOG 5
#define B_GRID 6
#define B_X 7
#define B_Y 8
#define B_Z 9
#define B_UNL 10
#define R_SLAVE 11

// This array of structures defines where each button bit is to be placed in the msg.
// It was also a convienient place to put the pin type initializer.
struct
{ byte pin;         //Arduino pin nbr
  byte ndx;         //message byte
  byte offsetmask;  //bitmask in byte (0== don't put in msg)
  byte mode;        //used at init time.
}bitmap[]=
{ {B_X,        1,0x80,INPUT_PULLUP},
  {B_Y,        1,0x40,INPUT_PULLUP},
  {B_Z,        1,0x20,INPUT_PULLUP},
  {B_SPINDLE,  2,0x80,INPUT_PULLUP},
  {B_FEEDRATE, 2,0x40,INPUT_PULLUP},
  {B_JOGRATE,  2,0x20,INPUT_PULLUP},
  {B_JOG,      3,0x10,INPUT_PULLUP},
  {B_GRID,     3,0x40,INPUT_PULLUP},
  {B_UNL,      3,0x20,INPUT_PULLUP},
  {R_SLAVE,0,0,OUTPUT}
};
static byte numbits=sizeof(bitmap)/sizeof(bitmap[0]);
static byte send_ascii=0;

struct
{ byte start;
  byte b[5];
  // byte checksum is computed and transmitted, not stored here 
}msg;
static byte i;

/*
 * init_slave resets the slave processor and sets up the
 * I2C
 */
void init_slave()
{
   digitalWrite(R_SLAVE,0); //pull down the slave's reset line
   delay(1000);
   digitalWrite(R_SLAVE,1);  //let the line back up
   delay(5000);
}

/* send_status sends the status message on the serial line to the BBB
 *  
 */
static byte *ptr;
static byte chksum=0;
void send_status()
{ 
  
 for(i=0,ptr= &msg.start, chksum=0;i<sizeof(msg);i++)
 { Serial.write(*ptr);  //send byte
   chksum+=*ptr++;
 }
 Serial.write(chksum);
}
/*
 * get_status builds the status message
 * It first queries the slave
 */
static byte mask;
void get_status()
{ 
  Wire.requestFrom(8,7);    //ask slave 8 for 7 bytes
  i=0;
  ptr=&msg.start;
  while(i<6)
  { if (Wire.available() )
    { *ptr++ = Wire.read();       //get the message from slave 1
      i++;
    }
  }
  msg.start=0xf0;
  //put our 9 bits into the message
  for(i=0;i<numbits;i++)
  { mask=bitmap[i].offsetmask;
    if(0!= mask)
      if(HIGH == digitalRead(bitmap[i].pin) )
        msg.b[bitmap[i].ndx]|=mask;
  }
  //Serial.print("calling send\n");
  send_status();
}
/*
 * Arduino setup entrypoint
 */
void setup() 
{
    Serial.begin(115200);
    for(i=0;i<numbits;i++)
      pinMode(bitmap[i].pin,bitmap[i].mode);
    Wire.begin();     //Initialize I2C on A4 & A5
    init_slave();
}
/*
 * Arduino loop entrypoint
 */
static int msg_cnt=0;

void loop() 
{
  byte chr;
  if(Serial.available())
  { chr=Serial.read();      //any input is treated as a "poll"
    get_status();
  }
}

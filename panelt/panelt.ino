/* Panelt--Dummy panel for debugging.
 *  --primary processor for the knobs and buttons on the CNC mill at Maker Nexus.
 * Copyright (C) Dan Clemmensen, 2019. All rights reserved. Licensed to you 
 *  under the GPLv3 license.
 * Control panel controller for the mn_cnc machine at Maker Nexus. The panel
 * has a bunch of dials and buttons. 
 * This is the simplified version to allow bench testing of a small number of buttons
 * and encoders using a single Arduino..
 */
//we do not include softserial because we are using the HW serial port on D0 and D1.


//only 2 button pins on benchtest Arduino
#define B_X 2
#define B_Y 3
//only 2 encoders on benchtest Arduino
#define B_MAIN_A 4
#define B_MAIN_B 5
#define B_JOG_A  6
#define B_JOG_B  7

struct
{ byte start;
  byte b[5]; 
}msg;
static byte i;

//  ttable --transition table for a quadrature encoder.
//  When turned clockwise, the cycle is A up, B up, A down, B down
//  For counterclockwise, the cycle is  A up, B down, A down, B up
//  The table gives the accum increment +1,-1, or 0 depending on the change
//  in A and B.
//  A and B should never both change at the same time. We code this as an "X"
//  which has value 0 so no increment occurs.
#define X 2
static byte ttable[4][4]=
{ //New state of AB, 
  // 00, 01, 10, 11
     { 0, -1,  1,  X}, //old state=00
     { 1,  0,  X, -1}, //old state=01
     {-1,  X,  0,  1}, //old state=10
     { X,  1, -1,  0}  //old state=11
};

typedef struct
{ byte accum;
  byte oldstate;
  byte ndx;         //message byte
  byte mask;        //bitmask in byte
  byte Apin;
  byte Bpin;
}ENCODER;

ENCODER encoders[]=
{ {0,0,0,0xff,B_MAIN_A,B_MAIN_B},
  //{0,0,1,0x1f,B_JOG_A, B_JOG_B}
};
static byte numencs=sizeof(encoders)/sizeof(encoders[0]);

/* update_encoder updates our internal accumulator for
 * an encoder given the accum and the A and B pin numbers
 */
static void update_encoder(ENCODER *enc)
{ byte ndx=enc->ndx;
  byte mask=enc->mask;
  byte a=HIGH==digitalRead(enc->Apin);
  byte b=HIGH==digitalRead(enc->Bpin);
  byte newstate=(a<<1) | b;
  byte inc=ttable[newstate][enc->oldstate];
  if(inc!=X)
  { enc->oldstate=newstate;
    enc->accum +=inc;
  }
  msg.b[ndx] &= ~mask;
  msg.b[ndx] |= enc->accum & mask;
}

// This array of structures defines where each button bit is to be placed in the msg.
// It was also a convienient place to put the pin type initializer.
struct
{ byte pin;         //Arduino pin nbr
  byte ndx;         //message byte
  byte offsetmask;  //bitmask in byte (0== don't put in msg)
  byte mode;        //used at init time.
}bitmap[]=
{ {B_X,     2,0x80,INPUT_PULLUP},
  {B_Y,     2,0x40,INPUT_PULLUP},
  {B_MAIN_A,3,0,INPUT_PULLUP},
  {B_MAIN_B,4,0,INPUT_PULLUP},
  {B_JOG_A, 2,0,INPUT_PULLUP},
  {B_JOG_B, 2,0,INPUT_PULLUP},
};
static byte numbits=sizeof(bitmap)/sizeof(bitmap[0]);
static byte send_ascii=0;


/* send_status sends the status message on the serial line to the BBB
 *  
 */
static byte *ptr;
static byte chksum=0;
void send_status()
{ for(i=0,ptr= &msg.start, chksum=0;i<sizeof(msg);i++)
    { Serial.write(*ptr);  //send byte
      chksum+=*ptr++;
    }
    Serial.write(chksum);
}
/* get_status builds the status message
 * It first queries the slave
 */
static byte mask;
void get_status()
{ msg.start=0xf0;
  //put our data into the message
  for(i=0;i<numbits;i++)
  { mask=bitmap[i].offsetmask;
    if(0!= mask)
      if(HIGH == digitalRead(bitmap[i].pin) )
        msg.b[bitmap[i].ndx]|= mask;
      else
        msg.b[bitmap[i].ndx] &= ~mask;
  }
  for(i=0;i<numencs;i++)
     update_encoder(&encoders[i]);
}
/* serialEvent is called from the RTOS when serial input is available
 */
void serialEvent() 
{
 byte chr;
 chr=Serial.read();      //any input is treated as a "poll"
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
}
/*
 * Arduino loop entrypoint
 */
static int msg_cnt=0;

void loop() 
{
  get_status();
}

/* Panel1--extension processor for the knobs and buttons on the CNC mill at Maker Nexus.
 * Copyright (C) Dan Clemmensen, 2019. All rights reserved. Licensed to you 
 *  under the GPLv3 license.
 *  
 * This Arduino, Ar1, is connected via I2C as a slave to Ar0.Together they comprise the
 * control panel controller for the mn_cnc machine at Maker Nexus. The panel
 * has a bunch of dials and buttons, including four encoder dials. Ar1 (this processor)
 * handles the four encoders and the four one-bit signals from each of two joysticks.
 * The encoder inputs are monitored continuously and the encoder accumulator values
 * are change whenver an encoder inpout changes.  It sends the accumulatores and the
 * joystick bits to Ar0 when polled.
 * 
 */
#include <Wire.h>


//we use lots of hard-wired inputs:
#define B_Zp 2
#define B_Zm 3
#define B_KNEEp 4
#define B_KNEEm 5
//
#define B_Xp 6
#define B_Xm 7
#define B_Yp 8
#define B_Ym 9
//
#define E_MAIN_A 10
#define E_MAIN_B 11
//
#define E_SPIN_A A0
#define E_SPIN_B A1
#define E_FEED_A A2
#define E_FEED_B A3
#define E_JOG_A  12
#define E_JOG_B  13

// This array of structures defines where each button is to be placed in the msg
struct
{ byte pin;         //Arduino pin nbr
  byte ndx;         //message byte
  byte offsetmask;  //bitmask in byte (0== don't put in msg)
  byte mode;        //used at init time.
}bitmap[]=
{ {B_Zp,   4,0x08,INPUT_PULLUP},
  {B_Zm,   4,0x04,INPUT_PULLUP},
  {B_KNEEp,4,0x02,INPUT_PULLUP},
  {B_KNEEm,4,0x01,INPUT_PULLUP},

  {B_Xp,   4,0x80,INPUT_PULLUP},
  {B_Xm,   4,0x40,INPUT_PULLUP},
  {B_Yp,   4,0x20,INPUT_PULLUP},
  {B_Ym,   4,0x10,INPUT_PULLUP},

  {E_MAIN_A,2,0,INPUT_PULLUP},
  {E_MAIN_B,2,0,INPUT_PULLUP},
  {E_SPIN_A,2,0,INPUT_PULLUP},
  {E_SPIN_B,2,0,INPUT_PULLUP},
  {E_FEED_A,2,0,INPUT_PULLUP},
  {E_FEED_B,2,0,INPUT_PULLUP},
  {E_JOG_A,2,0,INPUT_PULLUP},
  {E_JOG_B,2,0,INPUT_PULLUP}
};
byte numpins=sizeof(bitmap)/sizeof(bitmap[0]);
   

struct
{ byte start;
  byte b[5];
}msg;

typedef struct
{ int8_t accum;
  byte oldstate;
  byte Apin;
  byte Bpin;
  byte ndx;         //message byte
  byte mask;  //bitmask in byte
}ENCODER;

ENCODER encoders[]=
{ {0,0,E_MAIN_A,E_MAIN_B,0,0xff},   //all 8 bits of byte 0
  {1,0,E_SPIN_A,E_SPIN_B,1,0x1f},   //low 5 bits of byte 1
  {2,0,E_FEED_A,E_FEED_B,2,0x1f},   //low 5 bits of byte 2
  {3,0,E_JOG_A, E_JOG_B ,3,0x1f},   //low 5 bits of byte 3
};
static byte numencs=sizeof(encoders)/sizeof(encoders[0]);


/* update_encoder updates our internal accumulator for
 * an encoder given the accum and the A and B pin numbers
 */
//  ttable --transition table for a quadrature encoder.
//  When turned clockwise, the cycle is A up, B up, A down, B down
//  For counterclockwise, the cycle is  A up, B down, A down, B up
//  The table gives the accum increment +1,-1, or 0 depending on the change
//  in A and B.
//  A and B should never both change at the same time. We code this as an "X"
//  which has value 0 so no increment occurs.
#define X 2
static int8_t ttable[4][4]=
{ //New state of AB, 
  // 00, 01, 10, 11
  {   0, -1,  1,  X}, //old state=00
  {   1,  0,  X, -1}, //old state=01
  {  -1,  X,  0,  1}, //old state=10
  {   X,  1, -1,  0}  //old state=11
};
static void update_encoder(ENCODER *enc)
{ byte ndx=enc->ndx;
  byte mask=enc->mask;
  byte newstate=((HIGH==digitalRead(enc->Apin))<<1) | (HIGH==digitalRead(enc->Bpin));
  int8_t inc=ttable[enc->oldstate][newstate];
  if (inc!=X)
  { enc->accum +=inc;
    enc->oldstate=newstate;
  }
  msg.b[ndx] &= ~mask;
  msg.b[ndx] |= enc->accum & mask;
}
byte prior_encA;   //encoder input state
byte enc_count=0;  //encoder count

/*
 * reply_to_master. callback entrypojnt. called by Wire library
 * when the I2C master requests a message. builds and sends
 * the message. The encoder values are continually updated in the
 * message buffer from inside the main loop, so we do not touch them here.
 */
 void reply_to_master()
 {
  static byte i;
  static byte *ptr;
  static byte mask;
  //set the ordinary bits
  for(i=0;i<numpins;i++)
   { if(0!= (mask=bitmap[i].offsetmask))
      if(HIGH == digitalRead(bitmap[i].pin) )
        msg.b[bitmap[i].ndx]|=mask;
      else
        msg.b[bitmap[i].ndx]&=~mask;
  }
  Wire.write(&msg.start,7);
 }
 
/*
 * Arduino setup entrypoint
 */
void setup() 
{
  int i;
  for(i=0;i<numpins;i++)
    pinMode(bitmap[i].pin,bitmap[i].mode);
  // note: 8 is the first available slave address because 0-7 are reserved  
  Wire.begin(8);     //Initialize I2C as slave 8 on A4 & A5
  Wire.onRequest(reply_to_master);
}

/*
 * Arduino loop entrypoint. continually check the encoders and update the counts
 * We don't send data from this loop. It is sent from the callback function
 */
void loop() 
{
  int i;
  for(i=0;i<numencs;i++)
    update_encoder(&encoders[i]);    
}

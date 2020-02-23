#!/usr/bin/python
# userspace HAL component to receive input from the HW input panel on the CNC
# machine at Maker Nexus.
#Copyright (C) 2019 Dan Clemmensen, all rights reserved. Licences to you under
# the GPLv3.
# Together with an Arduino Nano (Ar1) acting as a port expander, an Arduino 
# Nano (Ar0) in the panel monitors the input from nine buttons, 2 joysticks,
# three pots, and an encoder wheel Ar0 consolidates the inputs into a single
# message and sends it on a serial-over-USB interface to this component.
# the component in turn sets its HAL "pins" to the appropriate values.
# The encoder wheel input requires special handling, as the operator
# uses buttons to select the current use of the wheel. 
#
# Apparently, the way a component sends "HW UI" control inputs to HAL
# is via the HALUI pins, so we'll try that.

import time, serial

#8-byte message received from HW panel. Set the HAL pins based on the
#contents
def process_panel_message(s):


#create the HAL "pins" we will control. These are inputs to HAL from us.
# For testing we just have hal "pin" for each bit and pot and the encoder.
# We will later need to do something about splitting the encoder to apply it
# to each axis.
h = hal.component("hwpanel")
h.newpin("Spindle",   hal.HAL_BIT,   hal.HAL_IN)
h.newpin("Feed_rate", hal.HAL_BIT,   hal.HAL_IN)
h.newpin("Jog_rate",  hal.HAL_BIT,   hal.HAL_IN)
h.newpin("jog",       hal.HAL_BIT,   hal.HAL_IN)
h.newpin("grid",      hal.HAL_BIT,   hal.HAL_IN)
h.newpin("X",         hal.HAL_BIT,   hal.HAL_IN)
h.newpin("Y",         hal.HAL_BIT,   hal.HAL_IN)
h.newpin("Z",         hal.HAL_BIT,   hal.HAL_IN)
h.newpin("unl",       hal.HAL_BIT,   hal.HAL_IN)
h.newpin("S_pot",     hal.HAL_FLOAT, hal.HAL_IN)
h.newpin("F_pot",     hal.HAL_FLOAT, hal.HAL_IN)
h.newpin("J_pot",     hal.HAL_FLOAT, hal.HAL_IN)
h.newpin("Zp",        hal.HAL_BIT,   hal.HAL_IN)
h.newpin("Zm",        hal.HAL_BIT,   hal.HAL_IN)
h.newpin("Ap",        hal.HAL_BIT,   hal.HAL_IN)
h.newpin("Am",        hal.HAL_BIT,   hal.HAL_IN)
h.newpin("Xp",        hal.HAL_BIT,   hal.HAL_IN)
h.newpin("Xm",        hal.HAL_BIT,   hal.HAL_IN)
h.newpin("Yp",        hal.HAL_BIT,   hal.HAL_IN)
h.newpin("Ym",        hal.HAL_BIT,   hal.HAL_IN)
h.newpin("enc",       hal.HAL_S32,   hal.HAL_IN)

#create HAL "pins" for outputs from HAL to us, if any.
#h.newpin("vfd-direction", hal.HAL_BIT, hal.HAL_OUT)

# initialize
ser=serial.Serial('/dev/ttyUSB0',115200)  #open the serial port
time.sleep(2)    #needed for some undocumented and unknown reason
start_chr= 0xf0
i=0
prior=0
print("done init\n")
#loop, receiving and processing messages from the panel
try:
  while True:
    ser.reset_input_buffer()
    ser.write(b'P')
    s=ser.read(8)
    if(s != prior):
      print(i,s.encode("hex"))
      prior=s
      process_panel_message(s)
    time.sleep(.1)
except KeyboardInterrupt:
  ser.close()
  raise SystemExit

    
    
    
            

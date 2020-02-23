#!/usr/bin/python
# userspace HAL component to set the speed of the spindle of the mn_cnc
# machine at Maker Nexus.
# Written as a user-space python compnent only because the  Adafruit library
# was the only reasonable worked example. Fortunately,  the timing is not
# critical because the actual pulses are generated in HW and the spindle speed
# changes are not real-time.
# This compinent combines the (nearly) independent
# functions of computiong the pulse-to-HZ VFD ratio, computing the
# machine HZ-to-RPM ratio, and actually setting the PWM pulse frequency.
# Uses the Adafruit BBIO library to control the PWM via the linux
# /sys/device pseudo file system.
# code written after examining
#   http://www.machinekit.io/docs/hal/halmodule/
# and
#   https://learn.adafruit.com/setting-up-io-python-library-on-beaglebone-black/pwm

#RPM is from the nameplate on the actual milling machine and are
# the RPM when the input is 60 HZ. The VFD is linear with an output of
# 60 HZ when the pulse rate is 10,000. So, we convert requested spindle
#speed S to pulse rate P as follows: P=S*R, where R is the ratio.
# the ratio for a particular gear and pulley is 10000/the nominal RPM for
# that gear and pulley speed.
#

import hal, time
import Adafruit_BBIO.PWM as PWM

h = hal.component("bbpwmgen")
h.newpin("gear-hi", hal.HAL_BIT, hal.HAL_IN)
h.newpin("pulley-A", hal.HAL_BIT, hal.HAL_IN)
h.newpin("pulley-B", hal.HAL_BIT, hal.HAL_IN)
h.newpin("pulley-C", hal.HAL_BIT, hal.HAL_IN)
h.newpin("pulley-D", hal.HAL_BIT, hal.HAL_IN)
h.newpin("speed", hal.HAL_FLOAT, hal.HAL_IN)
h.newpin("direction", hal.HAL_BIT, hal.HAL_IN)
h.newpin("bbbpin", hal.HAL_U32, hal.HAL_IN)
h.newpin("vfd-direction", hal.HAL_BIT, hal.HAL_OUT)
h.newpin("vfd-hz", hal.HAL_FLOAT, hal.HAL_OUT)  #debugging only


# initialize
old_pulley=2
old_gear=1
ratio=1
old_speed=1
old_direction=0
#RPM for [lo,hi] gear for pulley [A,B,C,D]
nameplate_RPM= [[180,1440],[280,2360],[440,3740],[650,5600]]
ratios=[]
for x in nameplate_RPM:
  ratios.append([10000/x[0],10000/x[1]])

#twelve valid BBB PWM pin numbers. The SoC HW cannot connect PWM to any
#other pins
pin_names= {813:"P8_13",819:"P8_14",834:"P8_34",836:"P8_36",845:"P8_45",
            914:"P9_14",916:"P9_16",921:"P9_21",922:"P9_22",928:"P9_28",
            929:"P9_29",931:"P9_31",942:"P9_42"
           }
bbbpin=0
h.ready()

while not(bbbpin in pin_names):
  time.sleep(0.1)
  bbbpin=h['bbbpin']
pin_name=pin_names[bbbpin]
PWM.start(pin_name,50,1)
#loop, checking for input changes and changing the frequency as needed
try:
  while True:
    time.sleep(0.1)
    pulley=2
    if h['pulley-A']:
      pulley=0
    if h['pulley-B']:
      pulley=1
    if h['pulley-C']:
      pulley=2
    if h['pulley-D']:
      pulley=3
    new_gear=h['gear-hi']
    new_speed=h['speed']
    new_direction=h['direction']
    if (pulley!=old_pulley) or (new_gear!= old_gear) or (new_speed!=old_speed) or (new_direction!=old_direction):
      old_gear=h['gear-hi']
      old_pulley=pulley
      old_speed=new_speed
      old_direction=new_direction
      ratio=(ratios[pulley])[new_gear]
      h['vfd-hz']=new_speed/60
      # Kludge. cannot set freq to 0, but at 1 HZ the VFD will stop
      speed=new_speed
      if speed == 0:
         speed=1
      PWM.set_frequency(pin_name,speed*ratio)
      h['vfd-direction']= new_direction ^ (not new_gear)
except KeyboardInterrupt:
  PWM.stop(pin_name)
  PWM.cleanup()
  raise SystemExit

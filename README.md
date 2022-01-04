# esp-adk-garage-remote

This is my second attempt at a garage door opener project.

The idea of the project is to control my community garage door with HomeKit, so
it can be opened from the car upon arrival, through Carplay dashboard (or Siri). 
Requirements are:
1. Can't install anything on the door itself
2. WiFi is not available at garage.
3. Must work with HomeKit

Because of 1. I am focusing on using an ESP32 devboard together with my 433MHz
remote control. ESP32 will close the circuit on the button of the remote and
keep it closed for 5 seconds. Originally, I thought the distance from my home
to the garage was too big for the remote to work. So my original idea was to
use the ESP32 due to its low power consumption vs Raspberry, so I could keep it
in the car (car 12V socket + battery powered). To solve 2., I would use a
HomeKit implementation with BLE transport. After trying the available BLE
implementation, I realized it does not support multiple HomeKit controller
pairings in its BLE transport. Since that is a requirement and no other BLE
implementations are available, this second attempt uses espressif's port of ADK
which unfortunately also does not support BLE, but it looks like the one more likely
to - maybe - implement it in the future. 

For the moment, I am using the IP transport. From the corner of my home, it
looks like the remote signal can reach the opener. So, while harder, it may be
posible to finish the project. I might look into a 433MHz transmitter module
to see if I can improve range, although that would imply getting the code of
the remote and see if I can copy, store and transmit it with a module. That 
would make this repo's implementation more complex, though.

### Disclaimer
This is my first ever project in C or any language with memory management.
I don't know what I am doing a lot of times, it is probably filled with
mistakes, memory leaks, format inconsistencies and inefficiencies.

If you're taking it as reference, please don't. If it hurts to look at it
and have any feedback or suggestions, please contact me, I will gladly
appreciate it.
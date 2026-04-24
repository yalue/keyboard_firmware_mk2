Updated Keyboard Firmware
=========================

This time with bluetooth! And a proper wakeup circuit! Uses the Adafruit
NRF52840 express. Well, two of them, one on the LHS and one on the RHS.

Goals:

 - Support bluetooth mode when on battery power and not plugged in.
 - Act as a normal USB keyboard when connected to USB.
 - Don't waste battery

Setup
-----

This uses the godawful arduino IDE, except version 1.8.whatever to make it
mildly less trash. You'll need to install the "Adafruit NRF52" board in the
board manager.

_Point your "sketchbook" location at this directory._ I made a modified version
of the "Keyboard" library from Arduino that I found a little less dumbed down.
It does things in a way I prefer, such as not sending a new packet when I don't
want to, and letting me manually deal with modifiers. And using keycodes rather
than ascii symbols.


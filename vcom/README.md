Virtual com port echo program.  This provides a com port interface over USB.
Also included is a test program for the host (tee).  To use this


cat somefile | tee /dev/cu.usbxxxx

this will send "somefile" through the com port where /dev/cu.usbxxx will have
to be determined by looking in /dev/ to see what port is created by the OS.


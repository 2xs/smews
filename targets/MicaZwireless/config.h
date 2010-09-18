#ifndef CONFIG_H_
#define CONFIG_H_


/***********************************************************************************************
* This file is used to set up node.
the application is like the radio counter of TinyOS.
the sender count from 0 to 7, each number is displayed on the Leds and sent to thereceiver.
the receiver shows this number using Leds.

* To rpepare the Hex file of the sender, you should change MY_ADDR in config.h to 0x1234 and NODE_TYPE to SENDER_NODE
* To rpepare the Hex file of the receiver, you should change MY_ADDR in config.h to 0x1235 and NODE_TYPE to RECEIVER_NODE

the for each file use this script to flash MicaZ mote :

uisp -v=0 -dpart=atmega128 -dprog=mib510 -dserial=/dev/ttyS4 --wr_fuse_e=0xff
uisp -v=0 -dpart=atmega128 -dprog=mib510 -dserial=/dev/ttyS4 --wr_fuse_e=0xff
uisp -v=0 -dpart=atmega128 -dprog=mib510 -dserial=/dev/ttyS4 --wr_fuse_h=0x9f
uisp -v=0 -dpart=atmega128 -dprog=mib510 -dserial=/dev/ttyS4 --wr_fuse_l=0xbf
uisp -v=0 -dpart=atmega128 -dprog=mib510 -dserial=/dev/ttyS4 --erase
uisp -v=0 -dpart=atmega128 -dprog=mib510 -dserial=/dev/ttyS4 --upload if=./driver_micaz.hex
uisp -v=0 -dpart=atmega128 -dprog=mib510 -dserial=/dev/ttyS4 --verify if=./driver_micaz.hex

************************************************************************************************/


#define CHANNEL             11
#define PANID               0x2420
#define RECEIVER_NODE		1
#define SENDER_NODE  		2

/***********************************************************************************************
* //address for receiver : 	0x1235
* //address for sender   :	0x1234
************************************************************************************************/
// to be set up for each mote
#define MY_ADDR             0x1235

#define NODE_TYPE	    RECEIVER_NODE




#endif




* This config.h file is used to set up the node.
the application is like the radio counter of TinyOS.
the sender count from 0 to 7, each number is displayed on the Leds and sent to the receiver.
the receiver shows this number using Leds.

* To rpepare the Hex file of the sender, you should change MY_ADDR in config.h to 0x1234 and NODE_TYPE to SENDER_NODE
* To rpepare the Hex file of the receiver, you should change MY_ADDR in config.h to 0x1235 and NODE_TYPE to RECEIVER_NODE

Please read Makefile to know how to compile and flash mote.

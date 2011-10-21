#IPADDR=134.206.11.3
IPADDR=192.168.100.200
#IPADDR=193.48.57.197
TARGET=mbed_ethernet
APPS=:welcome,ledmatrix,bandwidth
PROGRAM_CMD=cp bin/mbed_ethernet/smews.bin /media/MBED

all:
	scons ipaddr=$(IPADDR) target=$(TARGET) apps=$(APPS) && $(PROGRAM_CMD) && sync
clean:
	scons -c

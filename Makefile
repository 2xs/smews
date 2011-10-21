IPADDR=192.168.100.200
TARGET=mbed_ethernet
APPS=:welcome
PROGRAM_CMD=cp bin/mbed_ethernet/smews.bin /media/MBED

all:
	scons ipaddr=$(IPADDR) target=$(TARGET) apps=$(APPS) && $(PROGRAM_CMD) && sync
clean:
	scons -c

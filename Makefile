IPADDR=134.206.11.3
TARGET=mbed_ethernet
APPS=:welcome,bandwidth
PROGRAM_CMD=cp bin/mbed_ethernet/smews.bin /media/MBED

all:
	scons ipaddr=$(IPADDR) target=$(TARGET) apps=$(APPS) && $(PROGRAM_CMD) && sync
clean:
	scons -c
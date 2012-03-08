#
# Just a helper for Makefiles addicts.
# The smews build system uses scons
#
IPADDR=192.168.100.200
TARGET=mbed_ethernet
APPS=:welcome
PROGRAM_CMD=cp bin/mbed_ethernet/smews.bin /media/MBED

all:
	scons ipaddr=$(IPADDR) target=$(TARGET) apps=$(APPS) && $(PROGRAM_CMD) && sync
clean:
	scons -c

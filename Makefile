#
# Just a helper for Makefiles addicts.
# The smews build system uses scons
#
IPADDR=134.206.18.230
TARGET=mbed_ethernet
APPS=:welcome,ledmatrix,bandwidth
PROGRAM_CMD=cp bin/mbed_ethernet/smews.bin /media/MBED

all:
	scons ipaddr=$(IPADDR) target=$(TARGET) apps=$(APPS) && $(PROGRAM_CMD) && sync
clean:
	scons -c

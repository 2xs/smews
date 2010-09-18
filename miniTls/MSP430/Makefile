SHELL=/bin/sh
CC=msp430-gcc
CFLAGS=-mmcu=msp430x1611 --function-sections -c -Wall -O2
DEFINES=
LDFLAGS=-mmcu=msp430x1611 -Wl,--gc-sections,--undefined=_reset_vector__,--undefined=InterruptVectors,--undefined=_copy_data_init__,--undefined=_clear_bss_init__,--undefined=_end_of_init__
ECCSOURCES=ecc/curve_arithmetic.c ecc/gfp_arithmetic.c ecc/gfp_ops.c
TLSSOURCES=tls/tls.c tls/md5.c tls/sha1.c tls/prf.c tls/record.c tls/rc4.c tls/rand.c tls/s_cert.c
ECCOBJECTS=$(ECCSOURCES:.c=.o)
TLSOBJECTS=$(TLSSOURCES:.c=.o)

TARGET_SRC=tls-avr.c
TARGET_OBJ=$(TARGET_SRC:.c=.o)

MSP_ELF=bin/avr-tls.elf

all: msp

msp: $(MSP_ELF) 
$(MSP_ELF): $(ECCOBJECTS) $(TLSOBJECTS) $(TARGET_OBJ)
	$(CC) $(LDFLAGS) $(ECCOBJECTS) $(TLSOBJECTS) $(TARGET_OBJ) -o $@

$(ECCOBJECTS): $(ECCSOURCES)
$(TARGET_OBJ): $(TARGET_SRC)
$(TLSOBJECTS): $(TLSSOURCES)

size: 
	@echo "*********** MSP 430 *************";
	@echo TLS IMPLEMENTATION SIZE ;
		@size tls/*.o -t
	@echo ECC IMPLEMENTATION SIZE ;
		@size ecc/*.o -t
	@#@echo TARGET FILES
	@#	@size *.o -t

clean: 
	rm -rf ecc/*.o tls/*.o *.o bin/* *~

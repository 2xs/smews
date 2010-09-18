SHELL=/bin/sh
CC=avr-gcc
CFLAGS=--function-sections -c -Wall -O2 -mmcu=atmega128#$(DEFINES)
DEFINES= -DLINUX#-DDEBUG -DDEBUG_DEEP -DDEBUG_ECC
LDFLAGS= -mmcu=atmega128
ECCSOURCES=ecc/curve_arithmetic.c ecc/gfp_arithmetic.c ecc/gfp_ops.c
TLSSOURCES=tls/tls.c tls/md5.c tls/sha1.c tls/prf.c tls/record.c tls/rc4.c tls/rand.c tls/s_cert.c
ECCOBJECTS=$(ECCSOURCES:.c=.o)
TLSOBJECTS=$(TLSSOURCES:.c=.o)

TARGET_SRC=tls-avr.c
TARGET_OBJ=$(TARGET_SRC:.c=.o)

AVR_ELF=bin/avr-tls.elf

all: avr

avr: $(AVR_ELF) 
$(AVR_ELF): $(ECCOBJECTS) $(TLSOBJECTS) $(TARGET_OBJ)
	$(CC) $(LDFLAGS) $(ECCOBJECTS) $(TLSOBJECTS) $(TARGET_OBJ) -o $@

$(ECCOBJECTS): $(ECCSOURCES)
$(TARGET_OBJ): $(TARGET_SRC)
$(TLSOBJECTS): $(TLSSOURCES)

size: 
	@echo "*********** AVR  *************";
	@echo TLS IMPLEMENTATION SIZE ;
		@size tls/*.o -t
	@echo ECC IMPLEMENTATION SIZE ;
		@size ecc/*.o -t
	@#echo TARGET FILES
		@#size *.o -t

clean: 
	rm -rf ecc/*.o tls/*.o *.o bin/* *~

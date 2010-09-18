SHELL=/bin/sh
CC=gcc
CFLAGS=--function-sections -c -Wall -O2 $(DEFINES)
DEFINES= #-DDEBUG #-DDEBUG_DEEP -DDEBUG_ECC
LDFLAGS=
ECCSOURCES=ecc/curve_arithmetic.c ecc/gfp_arithmetic.c ecc/gfp_ops.c
TLSSOURCES=tls/tls.c tls/md5.c tls/sha1.c tls/prf.c tls/record.c tls/rc4.c tls/rand.c tls/s_cert.c
ECCOBJECTS=$(ECCSOURCES:.c=.o)
TLSOBJECTS=$(TLSSOURCES:.c=.o)

TARGET_SRC=tls-avr.c
TARGET_OBJ=$(TARGET_SRC:.c=.o)

LINUX_ELF=bin/linux-tls.elf

all: linux

linux: $(LINUX_ELF) 
$(LINUX_ELF): $(ECCOBJECTS) $(TLSOBJECTS) $(TARGET_OBJ)
	@$(CC) $(LDFLAGS) $(ECCOBJECTS) $(TLSOBJECTS) $(TARGET_OBJ) -o $@

$(ECCOBJECTS): $(ECCSOURCES)
$(TARGET_OBJ): $(TARGET_SRC)
$(TLSOBJECTS): $(TLSSOURCES)

size: 
	@echo "*********** LINUX DUMMY  *************";
	@echo TLS IMPLEMENTATION SIZE ;
		@size tls/*.o -t
	@echo ECC IMPLEMENTATION SIZE ;
		@size ecc/*.o -t
	@#echo TARGET FILES
		@#size *.o -t

clean: 
	rm -rf ecc/*.o tls/*.o *.o bin/* *~

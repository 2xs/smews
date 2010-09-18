SHELL=/bin/sh
CC=gcc
CFLAGS=-c -Wall -Os $(DEFINES)
DEFINES= -DDEBUG # -DDEBUG_ECC -DDEBUG_DEEP 
LDFLAGS=
ECCSOURCES=ecc/curve_arithmetic.c ecc/gfp_arithmetic.c ecc/gfp_ops.c
TLSSOURCES=tls/tls.c tls/md5.c tls/sha1.c tls/prf.c tls/record.c tls/rc4.c tls/rand.c tls/s_cert.c
ECCOBJECTS=$(ECCSOURCES:.c=.o)
TLSOBJECTS=$(TLSSOURCES:.c=.o)

LINUX_SRC=main.c hardware.c
LINUX_OBJ=$(LINUX_SRC:.c=.o)

LINUX_ELF=bin/linux-tls.elf
AVR_ELF=bin/avr-tls.elf


all: linux

linux: $(LINUX_ELF)
$(LINUX_ELF): $(ECCOBJECTS) $(TLSOBJECTS) $(LINUX_OBJ)
	$(CC) $(LDFLAGS) $(ECCOBJECTS) $(TLSOBJECTS) $(LINUX_OBJ) -o $@

$(ECCOBJECTS): $(ECCSOURCES)
$(LINUX_OBJ): $(LINUX_SRC)
$(TLSOBJECTS): $(TLSSOURCES)

size: 
	@echo "*********** Linux x86 *************";
	@echo TLS IMPLEMENTATION SIZE ;
		@size tls/*.o -t
	@echo ECC IMPLEMENTATION SIZE ;
		@size ecc/*.o -t
	@#echo TARGET FILES
		@#size *.o -t

clean: 
	rm -rf ecc/*.o tls/*.o *.o bin/* *~

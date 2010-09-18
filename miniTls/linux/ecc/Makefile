HELL=/bin/sh
CC=gcc
#AVR=avr-gcc
INCLUDE= #-I/ecc
CFLAGS=-c -Wall -Os #$(DEFINES)
#DEFINES= -DLINUX#-DDEBUG -DDEBUG_DEEP -DDEBUG_ECC
LDFLAGS=
ECCSOURCES=curve_arithmetic.c gfp_arithmetic.c
ECCOBJECTS=$(ECCSOURCES:.c=.o)

LINUX_SRC=main.c
LINUX_OBJ=$(LINUX_SRC:.c=.o)

LINUX_ELF=../bin/linux-tls.elf
AVR_ELF=../bin/avr-tls.elf


all: linux

avr: $(AVR_ELF) size
$(AVR_ELF): $(ECCOBJECTS) 
       # $(CC) $(LDFLAGS) $(ECCOBJECTS) -o $@

linux: $(LINUX_ELF) size
$(LINUX_ELF): $(ECCOBJECTS) $(LINUX_OBJ)
	$(CC) $(LDFLAGS) $(ECCOBJECTS) $(LINUX_OBJ) -o $@

$(ECCOBJECTS): $(ECCSOURCES)
$(LINUX_OBJ): $(LINUX_SRC)

size: 
	@echo ECC IMPLEMENTATION SIZE ;
	@size *.o -t
        

clean: 
	rm -rf *.o ../bin/* *~


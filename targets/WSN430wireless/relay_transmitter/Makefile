CC = msp430-gcc
CFLAGS = -mmcu=msp430x1611 -ffunction-sections

BINARIES = serie_to_radio
OBJ = serie_to_radio.o cc1100.o serial_line.o hardware.o spi1.o
all: $(BINARIES)

serie_to_radio:	$(OBJ)
	$(CC) $(CFLAGS) -o serie_to_radio $(OBJ)

serie_to_radio.o: serie_to_radio.c
	$(CC) -c $(CFLAGS) serie_to_radio.c
cc1100.o: cc1100.c
	$(CC) -c $(CFLAGS) cc1100.c
serial_line.o: serial_line.c
	$(CC) -c $(CFLAGS) serial_line.c
hardware.o: hardware.c
	$(CC) -c $(CFLAGS) hardware.c
spi1.o: spi1.c
	$(CC) -c $(CFLAGS) spi1.c

.PHONY: clean
clean:
	$(RM) $(BINARIES) *.o *~

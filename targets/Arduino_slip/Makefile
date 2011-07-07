MCU = atmega328p
TARGET = ../../bin/Arduino_slip/smews
PGMER = -c stk500v1 -b 57600 -P /dev/ttyUSB0
PGMERISP = -c avrispv2 -P /dev/ttyACM0 # pour UNO
DUDE = /usr/bin/avrdude -F -v -p $(MCU) -C /usr/local/arduino/arduino-0021/hardware/tools/avrdude.conf


upload:
	stty -F /dev/ttyUSB0 hupcl
	$(DUDE) $(PGMER) -U flash:w:$(TARGET).bin


size: 
	avr-size --format=avr --mcu=$(MCU) $(TARGET).elf


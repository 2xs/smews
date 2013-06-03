MCU = atmega328p
TERM = /dev/ttyUSB0 # for Duemilanove
#TERM = /dev/ttyACM0 # for UNO
TARGET = ../../bin/Arduino_ethernet/smews
PGMER = -c stk500v1 -b 57600 -P $(TERM) # For Duemilanove
#PGMER = -c stk500v1 -b 115200 -P $(TERM) # For Uno
DUDE = /usr/bin/avrdude -F -v -p $(MCU) -C /usr/local/arduino/arduino-0021/hardware/tools/avrdude.conf


upload:
	stty -F $(TERM) hupcl
	$(DUDE) $(PGMER) -U flash:w:$(TARGET).bin


size: 
	avr-size --format=avr --mcu=$(MCU) $(TARGET).elf


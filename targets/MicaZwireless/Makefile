# Hey Emacs, this is a -*- makefile -*-
#----------------------------------------------------------------------------
# WinAVR Makefile Template written by Eric B. Weddington, Jörg Wunsch, et al.
#
# Released to the Public Domain
#
# Additional material for this makefile was written by:
# Peter Fleury
# Tim Henigan
# Colin O'Flynn
# Reiner Patommel
# Markus Pfaff
# Sander Pool
# Frederik Rouleau
# Carlos Lamas
#
# Modified by Amdouni Mohamed
#----------------------------------------------------------------------------
# On command line:
#
# make all = Make software.
#
# make clean = Clean out built project files.
#
# make program = Download the hex file to the device, using uisp.
#                Please customize the serial port settings below first!
#
# To rebuild project do "make clean" then "make all".
#----------------------------------------------------------------------------

# MCU name
MCU = atmega128

# Processor frequency.
F_CPU = 7372800

# Output format. 
FORMAT = ihex

# Target file name (without extension).
TARGET = driver

# Object files directory
OBJDIR = .

# List C source files here. (C dependencies are automatically generated.)
SRC = $(TARGET).c mcu.c phy.c mac.c

# Optimization level, can be [0, 1, 2, 3, s]. 
OPT = s

# Place -D or -U options here for C sources
CDEFS = -DF_CPU=$(F_CPU)UL

#---------------- Compiler Options C ----------------
#  -O*:          optimization level
#  -Wall...:     warning level
#CFLAGS = -g$(DEBUG)
CFLAGS = $(CDEFS)
CFLAGS += -O$(OPT)
CFLAGS += -Wall

#---------------- Programming Options (uisp) ----------------
# should be changed in adequation with you serial port
# Programming hardware
SERIAL_OUT = /dev/ttyUSB0
USPI_FLAGS = -dpart=atmega128 -dprog=mib510 -dserial=$(SERIAL_OUT)

# Define programs and commands.
SHELL = sh
CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
REMOVE = rm -f
REMOVEDIR = rm -rf
COPY = cp

# Define all object files.
OBJ = $(SRC:%.c=$(OBJDIR)/%.o)

# Compiler flags to generate dependency files.
GENDEPFLAGS = -MMD -MP -MF .dep/$(@F).d

# Combine all necessary flags and optional flags.
# Add target processor to flags.
ALL_CFLAGS = -mmcu=$(MCU) -I. $(CFLAGS) $(GENDEPFLAGS)

#generated messages :
MSG_BEGIN = -------- begin --------
MSG_END = --------  end  --------
MSG_FLASH = Creating load file for Flash:
MSG_LINKING = Linking:
MSG_COMPILING = Compiling C:
MSG_CLEANING = Cleaning project:

# Default target.
all: begin gccversion build end

# Change the build target to build a HEX file or a library.
build: elf hex 
#build: lib


elf: $(TARGET).elf
hex: $(TARGET).hex

begin:
	@echo
	@echo $(MSG_BEGIN)

end:
	@echo $(MSG_END)
	@echo

# Display compiler version information.
gccversion : 
	@$(CC) --version

# Program the device.  
program: all
	uisp -v=0 $(USPI_FLAGS) --wr_fuse_e=0xff
	uisp -v=0 $(USPI_FLAGS) --wr_fuse_e=0xff
	uisp -v=0 $(USPI_FLAGS) --wr_fuse_h=0x9f
	uisp -v=0 $(USPI_FLAGS) --wr_fuse_l=0xbf
	uisp -v=0 $(USPI_FLAGS) --erase
	uisp -v=0 $(USPI_FLAGS) --upload if=./$(TARGET).hex
	uisp -v=0 $(USPI_FLAGS) --verify if=./$(TARGET).hex

# Create final output files (.hex, .eep) from ELF output file.
%.hex: %.elf
	@echo
	@echo $(MSG_FLASH) $@
	$(OBJCOPY) -O $(FORMAT) -R .eeprom -R .fuse -R .lock $< $@

# Link: create ELF output file from object files.
.SECONDARY : $(TARGET).elf
.PRECIOUS : $(OBJ)
%.elf: $(OBJ)
	@echo
	@echo $(MSG_LINKING) $@
	$(CC) $(ALL_CFLAGS) $^ --output $@ $(LDFLAGS)


# Compile: create object files from C source files.
$(OBJDIR)/%.o : %.c
	@echo
	@echo $(MSG_COMPILING) $<
	$(CC) -c $(ALL_CFLAGS) $< -o $@ 


# Target: clean project.
clean: begin clean_list end

clean_list :
	@echo
	@echo $(MSG_CLEANING)
	$(REMOVE) $(TARGET).hex
	$(REMOVE) $(TARGET).elf
	$(REMOVE) $(SRC:%.c=$(OBJDIR)/%.o)
	$(REMOVE) $(SRC:.c=.s)
	$(REMOVE) $(SRC:.c=.d)
	$(REMOVE) $(SRC:.c=.i)
	$(REMOVEDIR) .dep


# Create object files directory
$(shell mkdir $(OBJDIR) 2>/dev/null)


# Include the dependency files.
-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)



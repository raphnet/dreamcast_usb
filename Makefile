CC=avr-gcc
AS=$(CC)
LD=$(CC)
PROGNAME=dc_usb
CPU=atmega168

CFLAGS=-Wall -Os -Iusbdrv -I. -mmcu=$(CPU) -DF_CPU=16000000L #-DDEBUG_LEVEL=1 
LDFLAGS=-Wl,-Map=$(PROGNAME).map -mmcu=$(CPU) 
AVRDUDE=avrdude -p m168 -P usb -c avrispmkII

OBJS=usbdrv/usbdrv.o usbdrv/usbdrvasm.o main.o maplebus.o dc_pad.o

HEXFILE=$(PROGNAME).hex
ELFFILE=$(PROGNAME).elf

# symbolic targets:
all: $(HEXFILE)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

.S.o:
	$(CC) $(CFLAGS) -x assembler-with-cpp -c $< -o $@
# "-x assembler-with-cpp" should not be necessary since this is the default
# file type for the .S (with capital S) extension. However, upper case
# characters are not always preserved on Windows. To ensure WinAVR
# compatibility define the file type manually.

rxcode.asm: generate_rxcode.sh
	./generate_rxcode.sh > rxcode.asm

maplebus.o: maplebus.c rxcode.asm
	$(CC) $(CFLAGS) -c $< -o $@

.c.s:
	$(CC) $(CFLAGS) -S $< -o $@


clean:
	rm -f $(HEXFILE) $(PROGNAME).map $(PROGNAME).elf $(PROGNAME).hex *.o usbdrv/*.o main.s usbdrv/oddebug.s usbdrv/usbdrv.s

# file targets:
$(ELFFILE): $(OBJS)
	$(LD) $(LDFLAGS) -o $(ELFFILE) $(OBJS)

$(HEXFILE):	$(ELFFILE)
	rm -f $(HEXFILE) 
	avr-objcopy -j .text -j .data -O ihex $(ELFFILE) $(HEXFILE)
	avr-size $(ELFFILE)


flash: $(HEXFILE)
	$(AVRDUDE) -Uflash:w:$(HEXFILE) -B 1.0

# Extended fuse byte (Datasheet Table 28-5)
#
#  -  -  -  -  -  BOOTSZ1  BOOTSZ0  BOOTRST
#  0  0  0  0  0     0        0        1
#
EFUSE=0x01

# Fuse high byte (Datasheet Table 28-6)
#
# RSTDISBL  DWEN  SPIEN   WDTON  EESAVE  BODLEVEL2  BODLEVEL1  BODLEVEL0
#    1        1      0      1      0         1          0          1
#
HFUSE=0xd5

# Fuse low byte (Datasheet Table 28-7)
#
# CKDIV8   CKOUT   SUT1  SUT0  CKSEL3  CKSEL2  CKSEL1  CKSEL0
#    1        1      0     1      0      1       1       1
#
# Full swing crystal oscillator
# 0.4 - 20 MHz : CKSEL3..1  011
#
# Crystal Oscillator, BOD enabled (Table 9-6)
# CKSEL0  : 1
# SUT1..0 : 01
#
LFUSE=0xD7

fuse:
	$(AVRDUDE) -e -Uefuse:w:$(EFUSE):m -Uhfuse:w:$(HFUSE):m -Ulfuse:w:$(LFUSE):m -B 20.0 -v

chip_erase:
	$(AVRDUDE) -e -B 1.0 -F

reset:
	$(AVRDUDE) -B 1.0 -F
	

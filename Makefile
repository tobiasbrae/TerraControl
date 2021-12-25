CC=avr-gcc
MCU=atmega32
OBJCPY=avr-objcopy
PROGRAM=avrdude
PROGNAME=escTest
ELFFILE=$(basename $(PROGNAME)).elf
BINFILE=$(basename $(PROGNAME)).bin
OPTIMAZATION_FLAGS=-Os
CPU_FREQ=8000000UL
AVR_DUDE_CONF="C:\Program Files (x86)\Arduino\hardware\tools\avr\etc\avrdude.conf"

PROGDEVICE=COM9

OBJ=main.o display.o bitOperation.o

CFLAGS=-mmcu=${MCU} ${OPTIMAZATION_FLAGS} -DF_CPU=${CPU_FREQ} -std=c99 -Wall
LDFLAGS=-Wall

all: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJ) -o $(ELFFILE)
	$(OBJCPY) -O ihex $(ELFFILE) $(BINFILE)

%.o: %.c 
	${CC} ${CFLAGS} -c $<

install: $(OBJ)
	$(PROGRAM) -p m32 -c arduino -P $(PROGDEVICE) -b 19200 -C $(AVR_DUDE_CONF) -U flash:w:$(BINFILE)

clean:
	rm -f *.o $(ELFFILE) $(BINFILE)

complete:
	$(MAKE) clean
	$(MAKE)
	$(MAKE) install

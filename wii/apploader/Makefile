CROSS ?= powerpc-gekko-

#DEBUG = -DDEBUG
DEBUG =

ifeq ($(origin CC), default)
	CC = $(CROSS)gcc -m32
endif
ifeq ($(origin LD), default)
	LD = $(CROSS)ld
endif

OBJCOPY ?= $(CROSS)objcopy


CFLAGS = -Wall -W -Os -ffreestanding -mno-eabi -mno-sdata $(DEBUG)


blobs = apploader.bin

elfs  = apploader.elf
objs = start.o apploader.o


all: $(blobs)

%.o: %.c
	@echo "  COMPILE   $@"
	$(Q)$(CC) $(CFLAGS) -c -o $@ $< 

fwrite_patch.h: fwrite_patch.bin
	xxd -i fwrite_patch.bin > fwrite_patch.h

start.o: %.o: %.s
	$(CC) -c $< -o $@

$(blobs): %.bin: %.elf
	$(OBJCOPY) -Obinary $< $@

$(elfs): %.elf: apploader.lds $(objs)
	$(LD) -T $^ -o $@

clean:
	rm -f *.elf *.bin *.o

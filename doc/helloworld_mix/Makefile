# Makefile for helloworld.c

CC=arm-none-eabi-gcc 
LD=arm-none-eabi-ld
OBJCOPY=arm-none-eabi-objcopy

CFLAGS=-O2 -g
ASFLAGS=-O2 -g
LDFLAGS=-T helloworld.lds 


helloworld.bin: helloworld.elf
	$(OBJCOPY) -O binary $^ $@	

helloworld.elf: start.o helloworld.o
	$(LD) $(LDFLAGS) $^ -o $@	

start.o: start.s
	$(CC) $(CFLAGS) -c $^ -o $@

helloworld.o: helloworld.c
	$(CC) $(CFLAGS) -c $^ -o $@


.PHONY: clean 
clean:
	rm -rf helloworld.bin helloworld.elf start.o helloworld.o 


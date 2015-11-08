# Main Makefile

# =========================
# Setting up env. variables
# Project paths
# -------------
# PROJ_ROOT=/home/me/workspace/leeos
# export SRC_PATH=$(PROJ_ROOT)/src
# export TOOL_PATH=$(PROJ_ROOT)/tools

# Tool chain and flags
# --------------------
CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy

CFLAGS=-O2 -g
ASFLAGS=-O2 -g
LDFLAGS=-static -nostartfiles -nostdlib -Tkernel.lds -Ttext 0x30000000


# =======
# Targets
kernel=kernel.bin

kernel_objs=start.o abnormal.o init.o boot.o mmu.o print.o interrupt.o timer.o \
			memory.o driver.o ramdisk.o fs.o romfs.o exec.o syscall.o proc.o

ramdisk_img=ramdisk.img
romfs_img=romfs.img

app1=app1.elf
app2=app2.elf


# =============
# Defalt target
.PHONY: default
default: $(kernel) $(ramdisk_img) $(romfs_img) 
	@echo "\n========================="
	@echo "LeeOS Building Completed!"
	@echo "=========================\n"

# ----------------
# target $(kernel)
$(kernel): kernel.elf
	$(OBJCOPY) -O binary $^ $@	
# libgcc.a contains some basic operations for ARM, e.g., division 
kernel.elf: $(kernel_objs)
	$(CC) $(LDFLAGS) $^ -o $@ -lgcc 

# ---------------------
# target $(ramdisk_img)
$(ramdisk_img):
	@echo "\nCreating ramdisk image ..." 	
	dd if=/dev/zero of=$@ bs=1M count=2
	@echo "ramdisk image created!\n" 	

# -------------------
# target $(romfs_img)
$(romfs_img): $(app1) $(app2)
	@echo "\nCreating romfs image ..." 	
	mkdir -p tmp
	echo "0 1 2 3 4 5 6 7 8 9 " > tmp/number.txt
	cp $(app1) tmp	
	cp $(app2) tmp	
	genromfs -d tmp -f $@ 
	rm -r tmp
	dd if=$(romfs_img) of=$(ramdisk_img)	
	@echo "romfs image created!\n"

# --------------
# target $(app1)
# How to get .bin from .elf
#   objcopy -O binary app1.elf app1.bin 
$(app1):
	$(CC) -e main -nostartfiles -nostdlib -Ttext 0x30100000 -o $@ app1.c

# --------------
# target $(app2)
$(app2): 
	$(CC) -e main -nostartfiles -nostdlib -Ttext 0x30100000 -o $@ app2.c


# =========================
# Rules to generate objects
%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@
%.o: %.s
	$(CC) $(ASFLAGS) -c $^ -o $@


# =============
# Clean targets
.PHONY: clean 
clean:
	rm -rf $(kernel) kernel.elf $(kernel_objs) $(ramdisk_img) $(romfs_img) $(app1) $(app1_objs) $(app2) $(app2_objs)


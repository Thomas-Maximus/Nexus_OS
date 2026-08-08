# --- Nexus OS Makefile ---
# Built for WSL (x86_64 Ubuntu) targeting i686 Multiboot

CC = gcc
AS = nasm
LD = ld

# Flags for freestanding 32-bit kernel
CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector -fno-pic \
         -Ikernel -Ikernel/python -Ikernel/python/py -Ikernel/python/extmod \
         -Ikernel/python/shared/readline
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T kernel/linker.ld

# Source Files
SRCS_S = kernel/boot.s
SRCS_C = kernel/kernel.c \
         kernel/libc.c \
         kernel/drivers/vga.c \
         kernel/drivers/gdt.c \
         kernel/drivers/mem.c \
         kernel/drivers/idt.c \
         kernel/drivers/keyboard.c \
         kernel/initrd.c \
         kernel/python/nexus_hal.c

# MicroPython Core (Essential subset)
PY_FILES = $(wildcard kernel/python/py/*.c)

# Object Files
OBJS = kernel/boot.o \
       $(SRCS_C:.c=.o) \
       $(PY_FILES:.c=.o)

BIN = nexus_os.bin
INITRD = initrd.img

.PHONY: all clean run

all: $(BIN) $(INITRD)

%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $(BIN)

$(INITRD): nexus_ai.py tools/mkinitrd.py
	python3 tools/mkinitrd.py $(INITRD) nexus_ai.py

clean:
	find kernel -name "*.o" -type f -delete
	rm -f $(BIN) $(INITRD)

run: all
	qemu-system-x86_64 -kernel $(BIN) -initrd $(INITRD) -serial stdio

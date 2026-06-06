CC      := zig cc
CFLAGS  := -target x86_64-freestanding-none \
           -Wall -Wextra -ffreestanding -fno-stack-protector \
           -fpie -mno-red-zone -mno-mmx -mno-sse -mno-sse2 \
           -mcmodel=small -O2 \
           -fno-vectorize -fno-slp-vectorize \
           -I.
LD      := zig cc
LDFLAGS := -target x86_64-freestanding-none \
           -nostdlib -pie \
           -Wl,-T,linker.ld \
           -Wl,--no-dynamic-linker \
           -Wl,-e,kernel_main

NASM    := nasm
NASMFLAGS := -f elf64

SRCS    := kernel.c \
           string.c \
           modules/framebuffer/framebuffer.c \
           modules/mem/mem.c \
           modules/mem/pmm.c \
           modules/mem/paging.c \
           modules/mem/vmm.c \
           modules/interrupts/interrupts.c \
           modules/keyboard/keyboard.c \
           modules/timer/timer.c \
           modules/sysinfo/sysinfo.c \
           modules/startup_panel/startup_panel.c \
           modules/shell/shell.c

ASM_SRC := modules/interrupts/isr.asm

OBJS    := $(SRCS:.c=.o) $(ASM_SRC:.asm=.o)
TARGET  := kernel.elf

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.asm
	$(NASM) $(NASMFLAGS) -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

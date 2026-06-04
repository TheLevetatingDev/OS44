CC      := zig cc
CFLAGS  := -target x86_64-freestanding-none \
           -Wall -Wextra -ffreestanding -fno-stack-protector \
           -fpie -mno-red-zone -mno-mmx -mno-sse -mno-sse2 \
           -mcmodel=small -O2 \
           -fno-vectorize -fno-slp-vectorize
LD      := zig cc
LDFLAGS := -target x86_64-freestanding-none \
           -nostdlib -pie \
           -Wl,-T,linker.ld \
           -Wl,--no-dynamic-linker \
           -Wl,-e,kernel_main

SRCS    := kernel.c
OBJS    := $(SRCS:.c=.o)
TARGET  := kernel.elf

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
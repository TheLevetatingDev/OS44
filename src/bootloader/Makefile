CC      := zig cc
CFLAGS  := -target x86_64-uefi \
           -Wall -Wextra \
           -ffreestanding \
           -fno-stack-protector \
           -mno-red-zone \
           -O2
LD      := zig cc
LDFLAGS := -target x86_64-uefi \
           -nostdlib

SRCS    := bootloader.c
OBJS    := $(SRCS:.c=.o)
TARGET  := BOOTX64.EFI

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

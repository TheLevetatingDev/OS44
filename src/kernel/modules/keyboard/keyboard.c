#include "keyboard.h"
#include "../interrupts/interrupts.h"
#include "../framebuffer/framebuffer.h"
#include "../shell/shell.h"
#include <stdint.h>

static uint8_t shift_pressed = 0;
static uint8_t caps_lock = 0;

static const char scancode_to_ascii[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x',
    'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const char scancode_to_ascii_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|', 'Z', 'X',
    'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);

    if (scancode & 0x80) {
        uint8_t key = scancode & 0x7F;
        if (key == 0x2A || key == 0x36) shift_pressed = 0;
        return;
    }

    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = 1;
        return;
    }

    if (scancode == 0x3A) {
        caps_lock = !caps_lock;
        return;
    }

    if (scancode >= 128) return;

    char c = shift_pressed ? scancode_to_ascii_shift[scancode] : scancode_to_ascii[scancode];
    
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
        if (caps_lock) {
            if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
            else if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
        }
    }

    if (c != 0) {
        shell_putc(c);
    }
}

#define I8042_DATA    0x60
#define I8042_STATUS  0x64
#define I8042_CMD     0x64
#define I8042_STATUS_OBF  (1 << 0)
#define I8042_STATUS_IBF  (1 << 1)

static void i8042_wait_write(void) {
    int timeout = 100000;
    while (timeout-- > 0) {
        if (!(inb(I8042_STATUS) & I8042_STATUS_IBF)) return;
    }
}

static int i8042_wait_read(void) {
    int timeout = 100000;
    while (timeout-- > 0) {
        if (inb(I8042_STATUS) & I8042_STATUS_OBF) return 1;
    }
    return 0;
}

static void i8042_flush(void) {
    int limit = 16;
    while (limit-- > 0 && (inb(I8042_STATUS) & I8042_STATUS_OBF)) inb(I8042_DATA);
}

void keyboard_init(void) {
    i8042_wait_write(); outb(I8042_CMD, 0xAD);
    i8042_wait_write(); outb(I8042_CMD, 0xA7);
    i8042_flush();
    i8042_wait_write(); outb(I8042_CMD, 0x20);
    if (!i8042_wait_read()) return;
    uint8_t cfg = inb(I8042_DATA);
    cfg |= (1 << 0);
    cfg &= ~(1 << 1);
    cfg |= (1 << 6);
    i8042_wait_write(); outb(I8042_CMD, 0x60);
    i8042_wait_write(); outb(I8042_DATA, cfg);
    i8042_wait_write(); outb(I8042_CMD, 0xAB);
    if (i8042_wait_read()) {
        uint8_t result = inb(I8042_DATA);
        if (result != 0x00) return;
    } else {
        return;
    }
    i8042_wait_write(); outb(I8042_CMD, 0xAE);
    uint8_t mask = inb(PIC1_DATA);
    mask &= ~(1 << 1);
    outb(PIC1_DATA, mask);
}

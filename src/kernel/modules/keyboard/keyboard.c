#include "keyboard.h"
#include "modules/interrupts/interrupts.h"
#include "modules/framebuffer/framebuffer.h"
#include <stdint.h>

// Shell state
#define SHELL_BUFFER_SIZE 1024
const char SHELL_PROMPT[] = "> ";

static char shell_buffer[SHELL_BUFFER_SIZE];
static uint32_t shell_pos = 0;
static uint32_t shell_line = 0;
static uint8_t shift_pressed = 0;
static uint8_t caps_lock = 0;

// Keyboard scan code set 1
static const char scancode_to_ascii[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',  // 0x00-0x0F
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,  // 0x10-0x1F
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x',  // 0x20-0x2F
    'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0,          // 0x30-0x3F
    0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',          // 0x40-0x4F
    '2', '3', '0', '.', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                     // 0x50-0x5F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                             // 0x60-0x6F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0                              // 0x70-0x7F
};

static const char scancode_to_ascii_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',   // 0x00-0x0F
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,  // 0x10-0x1F
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|', 'Z', 'X', // 0x20-0x2F
    'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0, 0, 0, 0,          // 0x30-0x3F
    0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',          // 0x40-0x4F
    '2', '3', '0', '.', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                     // 0x50-0x5F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                             // 0x60-0x6F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0                              // 0x70-0x7F
};

static void shell_putchar(char c) {
    if (c == '\n') {
        shell_line++;
        shell_pos = 0;
        if (shell_line >= fb_height() / 16) {
            // Scroll up by one line (simplified - in reality you'd need to copy video memory)
            shell_line = fb_height() / 16 - 1;
            fb_clear(FB_COLOR_BLACK);
        }
        fb_draw_string(SHELL_PROMPT, 0, shell_line * 16, FB_COLOR_WHITE, FB_COLOR_BLACK);
        return;
    }

    if (shell_pos >= fb_width() / 8) {
        shell_line++;
        shell_pos = 0;
        if (shell_line >= fb_height() / 16) {
            shell_line = fb_height() / 16 - 1;
            fb_clear(FB_COLOR_BLACK);
        }
        fb_draw_string(SHELL_PROMPT, 0, shell_line * 16, FB_COLOR_WHITE, FB_COLOR_BLACK);
    }

    fb_draw_char(c, shell_pos * 8, shell_line * 16, FB_COLOR_WHITE, FB_COLOR_BLACK);
    shell_pos++;
}

static void shell_backspace(void) {
    if (shell_pos > 0) {
        shell_pos--;
        fb_draw_char(' ', shell_pos * 8, shell_line * 16, FB_COLOR_WHITE, FB_COLOR_BLACK);
    } else if (shell_line > 0) {
        shell_line--;
        shell_pos = fb_width() / 8 - 1;
        fb_draw_char(' ', shell_pos * 8, shell_line * 16, FB_COLOR_WHITE, FB_COLOR_BLACK);
    }
}

static void hex_to_string(char *buf, uint8_t value) {
    const char hex_chars[] = "0123456789ABCDEF";
    buf[0] = hex_chars[(value >> 4) & 0xF];
    buf[1] = hex_chars[value & 0xF];
    buf[2] = '\0';
}

void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);
    char scancode_str[3];
    hex_to_string(scancode_str, scancode);
    fb_draw_string("Scancode: ", 16, 266, FB_COLOR_WHITE, FB_COLOR_BLACK);
    fb_draw_string(scancode_str, 16 + (8 * 10), 266, FB_COLOR_WHITE, FB_COLOR_BLACK);

    if (scancode & 0x80) {
        // Key release
        uint8_t key = scancode & 0x7F;
        if (key == 0x2A || key == 0x36) { // Left or right shift
            shift_pressed = 0;
        }
        return;
    }

    // Key press
    if (scancode == 0x2A || scancode == 0x36) { // Left or right shift
        shift_pressed = 1;
        return;
    }

    if (scancode == 0x3A) { // Caps Lock
        caps_lock = !caps_lock;
        return;
    }

    if (scancode == 0x0E) { // Backspace
        shell_backspace();
        if (shell_pos > 0 && shell_buffer[shell_pos-1] != 0) {
            shell_buffer[--shell_pos] = 0;
        }
        return;
    }

    if (scancode == 0x1C) { // Enter
        shell_buffer[shell_pos] = '\n';
        shell_putchar('\n');
        shell_pos = 0;
        return;
    }

    if (scancode >= 128) return;

    char c = 0;
    if (shift_pressed) {
        c = scancode_to_ascii_shift[scancode];
    } else {
        c = scancode_to_ascii[scancode];
    }

    // Apply Caps Lock
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
        if (caps_lock) {
            if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
            else if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
        }
    }

    if (c != 0 && shell_pos < SHELL_BUFFER_SIZE - 1) {
        shell_buffer[shell_pos++] = c;
        shell_putchar(c);
    }
}

/* -----------------------------------------------------------------------
 * i8042 PS/2 controller initialisation
 * After UEFI ExitBootServices the controller may be left with:
 *   - keyboard port disabled
 *   - scan-code translation (set 2 -> set 1) turned off
 *   - stale bytes sitting in the output buffer
 * We must bring it up explicitly before IRQ1 will ever fire.
 * ----------------------------------------------------------------------- */

#define I8042_DATA    0x60
#define I8042_STATUS  0x64
#define I8042_CMD     0x64

#define I8042_STATUS_OBF  (1 << 0)  /* output buffer full  */
#define I8042_STATUS_IBF  (1 << 1)  /* input  buffer full  */

/* Wait until the controller is ready to accept a command/data byte. */
static void i8042_wait_write(void) {
    int timeout = 100000;
    while (timeout-- > 0) {
        if (!(inb(I8042_STATUS) & I8042_STATUS_IBF))
            return;
    }
}

/* Wait until the controller has a byte ready to be read. */
static int i8042_wait_read(void) {
    int timeout = 100000;
    while (timeout-- > 0) {
        if (inb(I8042_STATUS) & I8042_STATUS_OBF)
            return 1;
    }
    return 0; /* timed out */
}

/* Drain any stale bytes sitting in the output buffer. */
static void i8042_flush(void) {
    int limit = 16;
    while (limit-- > 0 && (inb(I8042_STATUS) & I8042_STATUS_OBF))
        inb(I8042_DATA);
}

void keyboard_init(void) {
    fb_draw_string("keyboard_init started.", 16, 282, FB_COLOR_WHITE, FB_COLOR_BLACK);
    /* 1. Disable both PS/2 ports so no IRQs fire during setup. */
    i8042_wait_write(); outb(I8042_CMD, 0xAD); /* disable keyboard port */
    i8042_wait_write(); outb(I8042_CMD, 0xA7); /* disable mouse port    */

    /* 2. Discard anything left in the output buffer. */
    i8042_flush();

    /* 3. Read the current controller configuration byte. */
    i8042_wait_write(); outb(I8042_CMD, 0x20);
    fb_draw_string("Reading PS/2 config byte...", 16, 320, FB_COLOR_WHITE, FB_COLOR_BLACK);
    if (!i8042_wait_read()) {
        fb_draw_string("PS/2 config read timeout!", 16, 336, FB_COLOR_RED, FB_COLOR_BLACK);
        return; // Timed out reading config
    }
    uint8_t cfg = inb(I8042_DATA);
    fb_draw_string("PS/2 config read OK.", 16, 320, FB_COLOR_GREEN, FB_COLOR_BLACK);


    /* 4. Modify config:
     *    bit 0 = keyboard IRQ1 enable     -> set
     *    bit 1 = mouse    IRQ12 enable     -> clear (don't need it)
     *    bit 6 = scan-code translation     -> SET (translate set 2 -> set 1)
     *            Our scancode table expects scan set 1; UEFI may have left
     *            translation off, which is why keypresses produced garbage
     *            or nothing at all on real hardware.
     */
    cfg |=  (1 << 0);  /* enable keyboard IRQ  */
    cfg &= ~(1 << 1);  /* disable mouse    IRQ */
    cfg |=  (1 << 6);  /* enable scan-code translation */
    i8042_wait_write(); outb(I8042_CMD, 0x60);
    i8042_wait_write(); outb(I8042_DATA, cfg);

    /* 5. Self-test the keyboard port (optional but catches broken firmware). */
    i8042_wait_write(); outb(I8042_CMD, 0xAB);
    fb_draw_string("Running keyboard self-test...", 16, 352, FB_COLOR_WHITE, FB_COLOR_BLACK);
    if (i8042_wait_read()) {
        uint8_t result = inb(I8042_DATA);
        if (result != 0x00) {
            fb_draw_string("Keyboard self-test FAILED!", 16, 368, FB_COLOR_RED, FB_COLOR_BLACK);
            return; // Self-test failed
        }
        fb_draw_string("Keyboard self-test PASSED.", 16, 352, FB_COLOR_GREEN, FB_COLOR_BLACK);
    } else {
        fb_draw_string("Keyboard self-test TIMEOUT!", 16, 368, FB_COLOR_RED, FB_COLOR_BLACK);
        return; // Timed out reading self-test result
    }

    /* 6. Re-enable the keyboard port. */
    i8042_wait_write(); outb(I8042_CMD, 0xAE);

    /* 7. Unmask IRQ1 on the PIC.
     *    (The PIC was left fully masked by intr_init; unmask only IRQ1.) */
    fb_draw_string("Unmasking IRQ1...", 16, 384, FB_COLOR_WHITE, FB_COLOR_BLACK);
    uint8_t mask = inb(PIC1_DATA);
    char mask_str[3];
    hex_to_string(mask_str, mask);
    fb_draw_string("PIC1_DATA before: ", 16, 400, FB_COLOR_WHITE, FB_COLOR_BLACK);
    fb_draw_string(mask_str, 16 + (8 * 18), 400, FB_COLOR_WHITE, FB_COLOR_BLACK);

    mask &= ~(1 << 1);
    hex_to_string(mask_str, mask);
    fb_draw_string("PIC1_DATA after:  ", 16, 416, FB_COLOR_WHITE, FB_COLOR_BLACK);
    fb_draw_string(mask_str, 16 + (8 * 18), 416, FB_COLOR_WHITE, FB_COLOR_BLACK);
    outb(PIC1_DATA, mask);

    fb_draw_string("keyboard_init finished.", 16, 298, FB_COLOR_WHITE, FB_COLOR_BLACK);
}

char keyboard_getchar(void) {
    // Simple implementation - in a real OS you'd have a buffer
    return 0;
}
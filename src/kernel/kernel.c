#include <stdint.h>
#include "bootinfo.h"
#include "modules/framebuffer/framebuffer.h"
#include "modules/mem/mem.h"
#include "modules/interrupts/interrupts.h"
#include "modules/keyboard/keyboard.h"

static void append_uint(char *buf, uint64_t *pos, uint64_t cap, uint64_t value) {
    char tmp[24];
    int len = 0;
    if (value == 0) {
        tmp[len++] = '0';
    } else {
        while (value > 0 && len < 24) {
            tmp[len++] = (char)('0' + (value % 10));
            value /= 10;
        }
    }
    while (len > 0 && *pos + 1 < cap) {
        buf[(*pos)++] = tmp[--len];
    }
}

static void format_ram_size(char *buf, uint64_t cap, uint64_t bytes) {
    uint64_t pos = 0;
    const char *prefix = "System RAM: ";
    for (uint64_t i = 0; prefix[i] && pos + 1 < cap; i++)
        buf[pos++] = prefix[i];

    if (bytes >= 1024 * 1024 * 1024) {
        append_uint(buf, &pos, cap, bytes / (1024 * 1024 * 1024));
        if (pos + 3 < cap) { buf[pos++] = ' '; buf[pos++] = 'G'; buf[pos++] = 'B'; }
    } else if (bytes >= 1024 * 1024) {
        append_uint(buf, &pos, cap, bytes / (1024 * 1024));
        if (pos + 3 < cap) { buf[pos++] = ' '; buf[pos++] = 'M'; buf[pos++] = 'B'; }
    } else if (bytes >= 1024) {
        append_uint(buf, &pos, cap, bytes / 1024);
        if (pos + 3 < cap) { buf[pos++] = ' '; buf[pos++] = 'K'; buf[pos++] = 'B'; }
    } else {
        append_uint(buf, &pos, cap, bytes);
        if (pos + 3 < cap) { buf[pos++] = ' '; buf[pos++] = 'B'; buf[pos++] = ' '; }
    }

    buf[pos < cap ? pos : cap - 1] = '\0';
}

void kernel_main(BootInfo *info) {
    if (!info || info->magic != BOOTINFO_MAGIC) {
        while (1) __asm__("hlt");
    }

    fb_init(info);
    intr_init();
    mem_init(info);
    keyboard_init();
    fb_draw_string("About to enable interrupts (sti).", 16, 314, FB_COLOR_WHITE, FB_COLOR_BLACK);
    __asm__ volatile("sti"); /* enable interrupts now that everything is ready */

    // fb_clear(FB_COLOR_BLACK);

    const uint64_t bar_height = 48;
    fb_draw_color_bar(0, bar_height);

    const uint64_t text_y = bar_height + 16;
    fb_draw_string("OS44", 32, text_y, FB_COLOR_WHITE, FB_COLOR_BLACK);
    fb_draw_string("Kernel booted successfully!", 32, text_y + 16, FB_COLOR_GREEN, FB_COLOR_BLACK);

    char *ram_line = mem_alloc(64);
    if (ram_line) {
        format_ram_size(ram_line, 64, mem_total_bytes());
        fb_draw_string(ram_line, 32, text_y + 32, FB_COLOR_WHITE, FB_COLOR_BLACK);
    }

    // Draw initial shell prompt
    fb_draw_string(SHELL_PROMPT, 0, 0, FB_COLOR_WHITE, FB_COLOR_BLACK);

    while (1) __asm__("hlt");
}

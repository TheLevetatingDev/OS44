#include <stdint.h>
#include "bootinfo.h"
#include "modules/framebuffer/framebuffer.h"
#include "modules/mem/mem.h"
#include "modules/interrupts/interrupts.h"
#include "modules/keyboard/keyboard.h"
#include "modules/timer/timer.h"
#include "modules/sysinfo/sysinfo.h"

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
    sysinfo_init(info);
    keyboard_init();
    timer_init(100);
    fb_draw_string("About to enable interrupts (sti).", 16, 314, FB_COLOR_WHITE, FB_COLOR_BLACK);
    __asm__ volatile("sti"); /* enable interrupts now that everything is ready */

    fb_clear(FB_COLOR_BLACK);
    // fb_draw_filled_circle(fb_width() / 2, fb_height() / 2, 100);
    fb_draw_color_bar(0, 48);

    const uint64_t text_y = 64;
    fb_draw_string("OS44", 32, text_y, FB_COLOR_WHITE, FB_COLOR_BLACK);
    fb_draw_string("Kernel booted successfully!", 32, text_y + 16, FB_COLOR_GREEN, FB_COLOR_BLACK);

    // Display system info
    char ram_str[64];
    format_ram_size(ram_str, 64, sysinfo_get_ram_total());
    fb_draw_string(ram_str, 32, text_y + 48, FB_COLOR_WHITE, FB_COLOR_BLACK);

    char cpu_str[64] = "CPU: ";
    sysinfo_get_cpu_brand(cpu_str + 5);
    fb_draw_string(cpu_str, 32, text_y + 64, FB_COLOR_WHITE, FB_COLOR_BLACK);

    char res_str[64] = "Res: ";
    uint64_t w, h;
    sysinfo_get_screen_res(&w, &h);
    uint64_t pos = 5;
    append_uint(res_str, &pos, 64, w);
    res_str[pos++] = 'x';
    append_uint(res_str, &pos, 64, h);
    res_str[pos] = '\0';
    fb_draw_string(res_str, 32, text_y + 80, FB_COLOR_WHITE, FB_COLOR_BLACK);

    // Simple loop to show uptime
    while (1) {
        char uptime_str[32];
        uint64_t pos = 0;
        const char *prefix = "Uptime: ";
        for (uint64_t i = 0; prefix[i] && pos + 1 < 32; i++)
            uptime_str[pos++] = prefix[i];
        
        append_uint(uptime_str, &pos, 32, timer_get_uptime_seconds());
        uptime_str[pos < 32 ? pos : 31] = '\0';
        
        // Clear previous uptime area
        for(uint64_t i=0; i<32*8; i+=8) fb_draw_char(' ', 32+i, text_y + 32, FB_COLOR_BLACK, FB_COLOR_BLACK);
        fb_draw_string(uptime_str, 32, text_y + 32, FB_COLOR_WHITE, FB_COLOR_BLACK);
        
        // Short delay
        for(uint64_t i=0; i<10000000; i++) __asm__ volatile("nop");
    }
}

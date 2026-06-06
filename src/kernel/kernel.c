#include <stdint.h>
#include "bootinfo.h"
#include "modules/framebuffer/framebuffer.h"
#include "modules/mem/mem.h"
#include "modules/mem/pmm.h"
#include "modules/interrupts/interrupts.h"
#include "modules/keyboard/keyboard.h"
#include "modules/timer/timer.h"
#include "modules/sysinfo/sysinfo.h"

static int pmm_test_status = 0; // 0: testing, 1: pass, 2: fail

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

static void run_pmm_test(void) {
    void *p1 = pmm_alloc_frame();
    void *p2 = pmm_alloc_frame();
    if (p1 && p2 && p1 != p2) {
        pmm_test_status = 1;
        pmm_free_frame(p1);
        pmm_free_frame(p2);
    } else {
        pmm_test_status = 2;
    }
}

static void render_screen(void) {
    fb_clear(FB_COLOR_BLACK);
    fb_draw_color_bar(0, 48);

    const uint64_t text_x = 32;
    uint64_t y = 64;

    fb_draw_string("OS44", text_x, y, FB_COLOR_WHITE, FB_COLOR_BLACK);
    y += 24;

    // Memory Info
    char mem_info[64] = "Total RAM: ";
    uint64_t pos = 11;
    append_uint(mem_info, &pos, 64, mem_total_bytes() / 1024 / 1024);
    for(int i=0; i<3; i++) mem_info[pos++] = " MB"[i];
    mem_info[pos] = '\0';
    fb_draw_string(mem_info, text_x, y, FB_COLOR_WHITE, FB_COLOR_BLACK);
    y += 16;
    
    char pmm_info[64] = "Free Pages: ";
    pos = 12;
    append_uint(pmm_info, &pos, 64, pmm_get_free_frames());
    pmm_info[pos] = '\0';
    fb_draw_string(pmm_info, text_x, y, FB_COLOR_WHITE, FB_COLOR_BLACK);
    y += 16;

    // PMM Test Status
    if (pmm_test_status == 1) {
        fb_draw_string("PMM Test: PASS", text_x, y, FB_COLOR_GREEN, FB_COLOR_BLACK);
    } else if (pmm_test_status == 2) {
        fb_draw_string("PMM Test: FAIL", text_x, y, FB_COLOR_RED, FB_COLOR_BLACK);
    } else {
        fb_draw_string("PMM Test: TESTING...", text_x, y, FB_COLOR_WHITE, FB_COLOR_BLACK);
    }
    y += 24;

    // CPU & Res Info
    char cpu_str[64] = "CPU: ";
    sysinfo_get_cpu_brand(cpu_str + 5);
    fb_draw_string(cpu_str, text_x, y, FB_COLOR_WHITE, FB_COLOR_BLACK);
    y += 16;

    char res_str[64] = "Res: ";
    uint64_t w, h;
    sysinfo_get_screen_res(&w, &h);
    pos = 5;
    append_uint(res_str, &pos, 64, w);
    res_str[pos++] = 'x';
    append_uint(res_str, &pos, 64, h);
    res_str[pos] = '\0';
    fb_draw_string(res_str, text_x, y, FB_COLOR_WHITE, FB_COLOR_BLACK);
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
    __asm__ volatile("sti");

    run_pmm_test();

    uint64_t last_tick = 0;
    while (1) {
        uint64_t current_tick = timer_get_uptime_seconds() * 25; // Simple tick simulation
        if (current_tick > last_tick) {
            render_screen();
            last_tick = current_tick;
        }
        __asm__ volatile("pause");
    }
}

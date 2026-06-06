#include <stdint.h>
#include "bootinfo.h"
#include "modules/framebuffer/framebuffer.h"
#include "modules/mem/mem.h"
#include "modules/mem/pmm.h"
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

void kernel_main(BootInfo *info) {
    if (!info || info->magic != BOOTINFO_MAGIC) {
        while (1) __asm__("hlt");
    }

    fb_init(info);
    intr_init();
    mem_init(info);
    sysinfo_init(info);

    fb_clear(FB_COLOR_BLACK);
    
    // Display memory stats
    char mem_info[64] = "Total RAM: ";
    uint64_t pos = 11;
    append_uint(mem_info, &pos, 64, mem_total_bytes() / 1024 / 1024);
    for(int i=0; i<3; i++) mem_info[pos++] = " MB"[i];
    mem_info[pos] = '\0';
    fb_draw_string(mem_info, 32, 64, FB_COLOR_WHITE, FB_COLOR_BLACK);
    
    char pmm_info[64] = "Free Pages: ";
    pos = 12;
    append_uint(pmm_info, &pos, 64, pmm_get_free_frames());
    pmm_info[pos] = '\0';
    fb_draw_string(pmm_info, 32, 80, FB_COLOR_WHITE, FB_COLOR_BLACK);

    // PMM Test
    fb_draw_string("Running PMM Test...", 32, 112, FB_COLOR_WHITE, FB_COLOR_BLACK);
    void *p1 = pmm_alloc_frame();
    void *p2 = pmm_alloc_frame();
    if (p1 && p2 && p1 != p2) {
        fb_draw_string("PMM Test: PASS", 32, 128, FB_COLOR_GREEN, FB_COLOR_BLACK);
        pmm_free_frame(p1);
        pmm_free_frame(p2);
    } else {
        fb_draw_string("PMM Test: FAIL", 32, 128, FB_COLOR_RED, FB_COLOR_BLACK);
    }

    // Simple loop...
    while (1) { __asm__ volatile("hlt"); }
}

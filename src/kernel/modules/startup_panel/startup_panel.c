#include "startup_panel.h"
#include "../framebuffer/framebuffer.h"
#include "../mem/mem.h"
#include "../mem/pmm.h"
#include "../sysinfo/sysinfo.h"
#include "../timer/timer.h"
#include <stdint.h>

static uint64_t frame_count = 0;

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

void startup_panel_render(int pmm_test_status) {
    frame_count = (frame_count + 1) % 1000;

    fb_clear(FB_COLOR_BLACK);
    fb_draw_color_bar(0, 48);

    const uint64_t text_x = 32;
    uint64_t y = 64;

    fb_draw_string("OS44", text_x, y, FB_COLOR_WHITE, FB_COLOR_BLACK);
    
    // Frame Counter (top right)
    char frame_str[32] = "Frame: ";
    uint64_t pos = 7;
    append_uint(frame_str, &pos, 32, frame_count);
    frame_str[pos] = '\0';
    fb_draw_string(frame_str, fb_width() - 150, y, FB_COLOR_WHITE, FB_COLOR_BLACK);
    y += 24;

    char mem_info[64] = "Total RAM: ";
    pos = 11;
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

    if (pmm_test_status == 1) {
        fb_draw_string("PMM Test: PASS", text_x, y, FB_COLOR_GREEN, FB_COLOR_BLACK);
    } else if (pmm_test_status == 2) {
        fb_draw_string("PMM Test: FAIL", text_x, y, FB_COLOR_RED, FB_COLOR_BLACK);
    } else {
        fb_draw_string("PMM Test: TESTING...", text_x, y, FB_COLOR_WHITE, FB_COLOR_BLACK);
    }
    y += 24;

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

    // Animated Loading Bar
    uint64_t bar_width = fb_width() - 64;
    uint64_t bar_height = 20;
    uint64_t bar_x = 32;
    uint64_t bar_y = fb_height() - 40;
    
    // Draw background/outline
    for(uint64_t i = 0; i < bar_width; i++) {
        fb_put_pixel(bar_x + i, bar_y, FB_COLOR_WHITE);
        fb_put_pixel(bar_x + i, bar_y + bar_height - 1, FB_COLOR_WHITE);
    }
    for(uint64_t j = 0; j < bar_height; j++) {
        fb_put_pixel(bar_x, bar_y + j, FB_COLOR_WHITE);
        fb_put_pixel(bar_x + bar_width - 1, bar_y + j, FB_COLOR_WHITE);
    }

    // Draw animated progress (smoother)
    uint64_t progress_width = (frame_count) * (bar_width - 4) / 1000;
    for(uint64_t i = 0; i < progress_width; i++) {
        for(uint64_t j = 0; j < bar_height - 2; j++) {
            fb_put_pixel(bar_x + 2 + i, bar_y + 1 + j, FB_COLOR_GREEN);
        }
    }
}

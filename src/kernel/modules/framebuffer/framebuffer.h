#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>
#include "../../bootinfo.h"

#define FB_COLOR_BLACK  0x00000000
#define FB_COLOR_WHITE  0x00FFFFFF
#define FB_COLOR_GREEN  0x0000FF00

void fb_init(BootInfo *info);
void fb_clear(uint32_t color);
void fb_put_pixel(uint64_t x, uint64_t y, uint32_t color);
uint32_t fb_rgb(uint8_t r, uint8_t g, uint8_t b);
void fb_draw_char(char c, uint64_t x, uint64_t y, uint32_t fg, uint32_t bg);
void fb_draw_string(const char *s, uint64_t x, uint64_t y, uint32_t fg, uint32_t bg);
void fb_draw_color_bar(uint64_t y, uint64_t height);
uint64_t fb_width(void);
uint64_t fb_height(void);

#endif

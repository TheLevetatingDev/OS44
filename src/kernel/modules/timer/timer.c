#include "timer.h"
#include <stdint.h>
#include "../interrupts/interrupts.h"
#include "../framebuffer/framebuffer.h"

#define PIT_COMMAND 0x43
#define PIT_CHANNEL0 0x40
#define INPUT_FREQ 1193182

static uint64_t timer_ticks = 0;

void timer_init(uint32_t frequency) {
    uint32_t divisor = INPUT_FREQ / frequency;
    outb(PIT_COMMAND, 0x36); // Command: Channel 0, lobyte/hibyte, rate generator
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));
}

void timer_handler(void) {
    timer_ticks++;
    if (timer_ticks % 100 == 0) {
        fb_draw_string("Tick!", 600, 0, FB_COLOR_WHITE, FB_COLOR_BLACK);
    }
}

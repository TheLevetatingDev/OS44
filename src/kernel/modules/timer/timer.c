#include "timer.h"
#include <stdint.h>
#include "../interrupts/interrupts.h"
#include "../framebuffer/framebuffer.h"

#define PIT_COMMAND 0x43
#define PIT_CHANNEL0 0x40
#define INPUT_FREQ 1193182

static volatile uint64_t timer_ticks = 0;
static volatile uint64_t system_uptime_seconds = 0;

void timer_init(uint32_t frequency) {
    uint32_t divisor = INPUT_FREQ / frequency;
    outb(PIT_COMMAND, 0x36); // Command: Channel 0, lobyte/hibyte, rate generator
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));

    // Unmask IRQ0
    uint8_t mask = inb(PIC1_DATA);
    mask &= ~(1 << 0);
    outb(PIC1_DATA, mask);
}

void timer_handler(void) {
    timer_ticks++;
    // Assuming frequency 100Hz
    if (timer_ticks % 100 == 0) {
        system_uptime_seconds++;
    }
}

uint64_t timer_get_uptime_seconds(void) {
    return system_uptime_seconds;
}

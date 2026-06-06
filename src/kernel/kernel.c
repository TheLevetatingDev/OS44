#include <stdint.h>
#include "bootinfo.h"
#include "modules/framebuffer/framebuffer.h"
#include "modules/mem/mem.h"
#include "modules/mem/pmm.h"
#include "modules/interrupts/interrupts.h"
#include "modules/keyboard/keyboard.h"
#include "modules/timer/timer.h"
#include "modules/sysinfo/sysinfo.h"
#include "modules/startup_panel/startup_panel.h"

static int pmm_test_status = 0; // 0: testing, 1: pass, 2: fail

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

void kernel_main(BootInfo *info) {
    if (!info || info->magic != BOOTINFO_MAGIC) {
        while (1) __asm__("hlt");
    }

    fb_init(info);
    intr_init();
    mem_init(info);
    fb_enable_double_buffering(); // Enable double buffering
    sysinfo_init(info);
    keyboard_init();
    timer_init(100); // 100Hz frequency
    __asm__ volatile("sti");

    run_pmm_test();

    uint64_t last_tick = 0;
    while (1) {
        uint64_t current_ticks = timer_get_ticks();
        // Update at ~25Hz (every 4 ticks, assuming 100Hz timer)
        if (current_ticks - last_tick >= 4) {
            startup_panel_render(pmm_test_status);
            fb_swap_buffers(); // Swap buffers
            last_tick = current_ticks;
        }
        __asm__ volatile("pause");
    }
}

#include <stdint.h>
#include "bootinfo.h"
#include "modules/framebuffer/framebuffer.h"
#include "modules/mem/mem.h"
#include "modules/mem/pmm.h"
#include "modules/mem/vmm.h"
#include "modules/interrupts/interrupts.h"
#include "modules/keyboard/keyboard.h"
#include "modules/timer/timer.h"
#include "modules/sysinfo/sysinfo.h"
#include "modules/process/process.h"
#include "modules/ide/ide.h"
#include "modules/pci/pci.h"
#include "modules/debug_panel/debug_panel.h"

#define COM1 0x3F8

static void serial_init(void) {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
}

static void serial_putc(char c) {
    while ((inb(COM1 + 5) & 0x20) == 0);
    outb(COM1, c);
}

static void serial_puts(const char *s) {
    while (*s) {
        if (*s == '\n') serial_putc('\r');
        serial_putc(*s++);
    }
}

static int pmm_test_status = 0;
static int vmm_test_status = 0;

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

static void run_vmm_test(void) {
    void *v1 = vmm_alloc(4096);
    if (v1) {
        *(uint64_t *)v1 = 0xDEADBEEFCAFEBABE;
        if (*(uint64_t *)v1 == 0xDEADBEEFCAFEBABE) {
            vmm_test_status = 1;
            vmm_free(v1);
        } else {
            vmm_test_status = 2;
        }
    } else {
        vmm_test_status = 2;
    }
}

void kernel_main(BootInfo *info) {
    serial_init();
    serial_puts("\n=== OS44 KERNEL ===\n");

    if (!info || info->magic != BOOTINFO_MAGIC) {
        serial_puts("FATAL: bad bootinfo\n");
        while (1) __asm__("hlt");
    }
    serial_puts("[1] bootinfo OK\n");

    fb_init(info);
    serial_puts("[2] fb_init OK\n");

    intr_init();
    serial_puts("[3] intr_init OK\n");

    mem_init(info);
    serial_puts("[4] mem_init OK\n");

    fb_enable_double_buffering();
    serial_puts("[5] double buffering OK\n");

    sysinfo_init(info);
    serial_puts("[6] sysinfo_init OK\n");

    keyboard_init();
    serial_puts("[7] keyboard_init OK\n");

    timer_init(100);
    serial_puts("[8] timer_init OK\n");

    serial_puts("[9] calling ide_init...\n");
    ide_init();
    serial_puts("[10] ide_init DONE\n");

    serial_puts("[11] calling pci_init...\n");
    pci_init();
    serial_puts("[12] pci_init DONE\n");
    { int x = pci_device_count; (void)x; }

    serial_puts("[13] calling process_init...\n");
    process_init();
    serial_puts("[14] process_init DONE\n");

    serial_puts("[14.5] about to sti\n");
    __asm__ volatile("sti");
    serial_puts("[15] interrupts enabled\n");

    run_pmm_test();
    serial_puts("[16] pmm test done\n");

    run_vmm_test();
    serial_puts("[17] vmm test done\n");

    debug_panel_init(pmm_test_status, vmm_test_status);
    serial_puts("[18] debug_panel_init done\n");

    uint64_t loop_count = 0;
    uint64_t last_tick = 0;
    while (1) {
        uint64_t current_ticks = timer_get_ticks();
        scheduler();

        if (current_ticks - last_tick >= 2) {
            debug_panel_render();
            fb_swap_buffers();
            last_tick = current_ticks;
        }

        loop_count++;
        if (loop_count % 5000000 == 0) {
            serial_puts("[LOOP] alive, ticks=");
            char tbuf[21];
            uint64_t t = current_ticks;
            int i = 0;
            if (t == 0) { tbuf[i++] = '0'; }
            else {
                char tmp[20]; int j = 0;
                while (t > 0) { tmp[j++] = '0' + (t % 10); t /= 10; }
                while (j > 0) tbuf[i++] = tmp[--j];
            }
            tbuf[i] = '\0';
            serial_puts(tbuf);
            serial_puts("\n");
        }

        __asm__ volatile("pause");
    }
}

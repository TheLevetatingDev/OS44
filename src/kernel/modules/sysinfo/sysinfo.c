#include "sysinfo.h"
#include "../mem/mem.h"

static BootInfo *boot_info;

void sysinfo_init(BootInfo *info) {
    boot_info = info;
}

void sysinfo_get_cpu_brand(char *buf) {
    uint32_t *dest = (uint32_t *)buf;

    for (uint32_t i = 0x80000002; i <= 0x80000004; i++) {
        uint32_t eax, ebx, ecx, edx;
        __asm__ volatile("cpuid"
                         : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                         : "a"(i));
        *dest++ = eax;
        *dest++ = ebx;
        *dest++ = ecx;
        *dest++ = edx;
    }
    buf[48] = '\0';
}

uint64_t sysinfo_get_ram_total(void) {
    return mem_total_bytes();
}

void sysinfo_get_screen_res(uint64_t *w, uint64_t *h) {
    if (boot_info) {
        *w = boot_info->fb_width;
        *h = boot_info->fb_height;
    } else {
        *w = 0;
        *h = 0;
    }
}

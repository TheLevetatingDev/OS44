#include "sysinfo.h"
#include "../mem/mem.h"
#include "../mem/pmm.h"
#include "../timer/timer.h"
#include "../../string.h"

static BootInfo *boot_info;
static char cpu_vendor_str[16];
static char cpu_brand_str[49];
static uint32_t cpu_features_edx;
static uint32_t cpu_features_ecx;

void sysinfo_init(BootInfo *info) {
    boot_info = info;

    uint32_t eax, ebx, ecx, edx;

    __asm__ volatile("cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0));
    *((uint32_t *)cpu_vendor_str + 0) = ebx;
    *((uint32_t *)cpu_vendor_str + 1) = edx;
    *((uint32_t *)cpu_vendor_str + 2) = ecx;
    cpu_vendor_str[12] = '\0';

    __asm__ volatile("cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1));
    cpu_features_edx = edx;
    cpu_features_ecx = ecx;

    uint32_t *dest = (uint32_t *)cpu_brand_str;
    for (uint32_t i = 0x80000002; i <= 0x80000004; i++) {
        __asm__ volatile("cpuid"
            : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
            : "a"(i));
        *dest++ = eax;
        *dest++ = ebx;
        *dest++ = ecx;
        *dest++ = edx;
    }
    cpu_brand_str[48] = '\0';
}

void sysinfo_get_cpu_vendor(char *buf) {
    memcpy(buf, cpu_vendor_str, 13);
}

void sysinfo_get_cpu_brand(char *buf) {
    memcpy(buf, cpu_brand_str, 49);
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

void sysinfo_get_os_version(char *buf) {
    memcpy(buf, "OS44 v0.1", 10);
}

void sysinfo_get_uefi_vendor(char *buf) {
    for (int i = 0; i < 32; i++) {
        buf[i] = (char)boot_info->fw_vendor[i];
    }
    buf[32] = '\0';
}

uint32_t sysinfo_get_uefi_revision(void) {
    return boot_info->fw_revision;
}

void sysinfo_get_uptime(uint64_t *seconds) {
    *seconds = timer_get_uptime_seconds();
}

void sysinfo_get_mem_usage(uint64_t *used, uint64_t *total) {
    *total = mem_total_bytes();
    uint64_t free_frames = pmm_get_free_frames();
    uint64_t used_frames = (*total / 4096) - free_frames;
    *used = used_frames * 4096;
}

uint64_t sysinfo_get_cpu_features_edx(void) {
    return cpu_features_edx;
}

uint64_t sysinfo_get_cpu_features_ecx(void) {
    return cpu_features_ecx;
}

uint32_t sysinfo_get_lapic_id(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1));
    return (ebx >> 24) & 0xFF;
}

uint32_t sysinfo_get_core_count(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1));
    return ((ebx >> 16) & 0xFF);
}

void sysinfo_get_cpu_features(char *buf, uint64_t cap) {
    int pos = 0;
    uint64_t edx = cpu_features_edx;
    uint64_t ecx = cpu_features_ecx;
    uint64_t features = (edx & 0xFFFFFFFF) | ((ecx & 0xFFFFFFFF) << 32);

    if (cap == 0) cap = 0xFFFFFFFFFFFFFFFF;

    if ((features & (1 << 0)) && (pos + 3 < (int)cap)) { memcpy(buf + pos, "FPU ", 4); pos += 4; }
    if ((features & (1 << 1)) && (pos + 4 < (int)cap)) { memcpy(buf + pos, "VME  ", 5); pos += 5; }
    if ((features & (1 << 2)) && (pos + 4 < (int)cap)) { memcpy(buf + pos, "DE   ", 5); pos += 5; }
    if ((features & (1 << 3)) && (pos + 4 < (int)cap)) { memcpy(buf + pos, "PSE  ", 5); pos += 5; }
    if ((features & (1 << 4)) && (pos + 4 < (int)cap)) { memcpy(buf + pos, "TSC  ", 5); pos += 5; }
    if ((features & (1 << 5)) && (pos + 4 < (int)cap)) { memcpy(buf + pos, "MSR  ", 5); pos += 5; }
    if ((features & (1 << 8)) && (pos + 5 < (int)cap)) { memcpy(buf + pos, "CX8  ", 5); pos += 5; }
    if ((features & (1 << 9)) && (pos + 5 < (int)cap)) { memcpy(buf + pos, "APIC ", 5); pos += 5; }
    if ((features & (1 << 11)) && (pos + 5 < (int)cap)) { memcpy(buf + pos, "SEP  ", 5); pos += 5; }
    if ((features & (1 << 15)) && (pos + 5 < (int)cap)) { memcpy(buf + pos, "CMOV ", 5); pos += 5; }
    if ((features & (1 << 23)) && (pos + 5 < (int)cap)) { memcpy(buf + pos, "MMX  ", 5); pos += 5; }
    if ((features & (1 << 24)) && (pos + 5 < (int)cap)) { memcpy(buf + pos, "FXSR ", 5); pos += 5; }
    if ((features & (1 << 25)) && (pos + 4 < (int)cap)) { memcpy(buf + pos, "SSE  ", 4); pos += 4; }
    if ((features & (1 << 26)) && (pos + 5 < (int)cap)) { memcpy(buf + pos, "SSE2 ", 5); pos += 5; }

    if ((features & (1LL << 33)) && (pos + 5 < (int)cap)) { memcpy(buf + pos, "SSE3 ", 5); pos += 5; }
    if ((features & (1LL << 35)) && (pos + 6 < (int)cap)) { memcpy(buf + pos, "FMA  ", 5); pos += 5; }
    if ((features & (1LL << 36)) && (pos + 7 < (int)cap)) { memcpy(buf + pos, "CX16 ", 6); pos += 6; }
    if ((features & (1LL << 41)) && (pos + 7 < (int)cap)) { memcpy(buf + pos, "SSE41 ", 6); pos += 6; }
    if ((features & (1LL << 42)) && (pos + 7 < (int)cap)) { memcpy(buf + pos, "SSE42 ", 6); pos += 6; }
    if ((features & (1LL << 43)) && (pos + 5 < (int)cap)) { memcpy(buf + pos, "X2APIC", 6); pos += 6; }
    if ((features & (1LL << 57)) && (pos + 4 < (int)cap)) { memcpy(buf + pos, "AVX  ", 4); pos += 4; }
    if ((features & (1LL << 58)) && (pos + 5 < (int)cap)) { memcpy(buf + pos, "AVX2 ", 5); pos += 5; }
    if ((features & (1LL << 62)) && (pos + 7 < (int)cap)) { memcpy(buf + pos, "AVX512", 6); pos += 6; }

    if (pos > 0 && buf[pos - 1] == ' ') pos--;
    buf[pos] = '\0';
}

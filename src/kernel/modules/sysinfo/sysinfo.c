#include "sysinfo.h"
#include "../mem/mem.h"
#include "../mem/pmm.h"
#include "../timer/timer.h"
#include "../../string.h"

// Check if strlen exists in "../../string.h". If not, add a local one or declare it.
size_t strlen(const char *str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

static BootInfo *boot_info;

static void itoa(uint64_t val, char *buf) {
    char *p = buf;
    if (val == 0) {
        *p++ = '0';
        *p = '\0';
        return;
    }
    char tmp[20];
    int i = 0;
    while (val > 0) {
        tmp[i++] = (val % 10) + '0';
        val /= 10;
    }
    while (i > 0) *p++ = tmp[--i];
    *p = '\0';
}

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

void sysinfo_get_os_version(char *buf) {
    memcpy(buf, "OS44 v0.1 Phynx", 16);
}

void sysinfo_get_uefi_info(char *buf) {
    // Vendor Name
    for (int i=0; i<32; i++) {
        buf[i] = (char)boot_info->fw_vendor[i];
    }
    buf[32] = ' ';
    // Revision
    char rev[10];
    itoa(boot_info->fw_revision, rev);
    memcpy(buf+33, rev, 10);
    // Useful UEFI info: add memory map size/desc size
    memcpy(buf+43, " | MMap:", 8);
    char mmap_s[10];
    itoa(boot_info->mmap_size, mmap_s);
    memcpy(buf+51, mmap_s, 10);
}

void sysinfo_get_uptime(uint64_t *seconds) {
    *seconds = timer_get_uptime_seconds();
}

void sysinfo_get_mem_usage(uint64_t *used, uint64_t *total) {
    *total = mem_total_bytes();
    // Assuming 4KB frames
    uint64_t free_frames = pmm_get_free_frames();
    uint64_t used_frames = (*total / 4096) - free_frames;
    *used = used_frames * 4096;
}

// Helper functions for string appending
static void append_str(char *buf, int *pos, const char *s) {
    size_t len = strlen(s);
    memcpy(buf + *pos, s, len);
    *pos += len;
}

static void append_val(char *buf, int *pos, uint64_t val) {
    char s[20];
    itoa(val, s);
    append_str(buf, pos, s);
}

void sysinfo_get_sysinfo_string(char *buf) {
    char os_ver[32], cpu[50], uefi[64];
    sysinfo_get_os_version(os_ver);
    sysinfo_get_cpu_brand(cpu);
    sysinfo_get_uefi_info(uefi);
    
    uint64_t uptime;
    sysinfo_get_uptime(&uptime);
    
    uint64_t mem_u, mem_t;
    sysinfo_get_mem_usage(&mem_u, &mem_t);
    
    uint64_t w, h;
    sysinfo_get_screen_res(&w, &h);
    
    int pos = 0;
    
    append_str(buf, &pos, "OS:"); append_str(buf, &pos, os_ver);
    append_str(buf, &pos, "|CPU:"); append_str(buf, &pos, cpu);
    append_str(buf, &pos, "|MEM:"); append_val(buf, &pos, mem_u/1024/1024); append_str(buf, &pos, "MB/"); append_val(buf, &pos, mem_t/1024/1024); append_str(buf, &pos, "MB");
    append_str(buf, &pos, "|UP:"); append_val(buf, &pos, uptime); append_str(buf, &pos, "s");
    append_str(buf, &pos, "|RES:"); append_val(buf, &pos, w); append_str(buf, &pos, "x"); append_val(buf, &pos, h);
    append_str(buf, &pos, "|UEFI:"); append_str(buf, &pos, uefi);
    buf[pos] = '\0';
}

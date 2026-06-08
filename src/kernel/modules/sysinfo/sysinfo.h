#ifndef SYSINFO_H
#define SYSINFO_H

#include <stdint.h>
#include "../../bootinfo.h"

void sysinfo_init(BootInfo *info);
void sysinfo_get_cpu_brand(char *buf);
uint64_t sysinfo_get_ram_total(void);
void sysinfo_get_screen_res(uint64_t *w, uint64_t *h);
void sysinfo_get_os_version(char *buf);
void sysinfo_get_uefi_info(char *buf);
void sysinfo_get_uptime(uint64_t *seconds);
void sysinfo_get_mem_usage(uint64_t *used, uint64_t *total);

// New
void sysinfo_get_sysinfo_string(char *buf);

#endif

#ifndef SYSINFO_H
#define SYSINFO_H

#include <stdint.h>
#include "../../bootinfo.h"

void sysinfo_init(BootInfo *info);
void sysinfo_get_cpu_vendor(char *buf);
void sysinfo_get_cpu_brand(char *buf);
uint64_t sysinfo_get_ram_total(void);
void sysinfo_get_screen_res(uint64_t *w, uint64_t *h);
void sysinfo_get_os_version(char *buf);
void sysinfo_get_uefi_vendor(char *buf);
uint32_t sysinfo_get_uefi_revision(void);
void sysinfo_get_uptime(uint64_t *seconds);
void sysinfo_get_mem_usage(uint64_t *used, uint64_t *total);
void sysinfo_get_cpu_features(char *buf, uint64_t cap);
uint64_t sysinfo_get_cpu_features_edx(void);
uint64_t sysinfo_get_cpu_features_ecx(void);
uint32_t sysinfo_get_lapic_id(void);
uint32_t sysinfo_get_core_count(void);

#endif

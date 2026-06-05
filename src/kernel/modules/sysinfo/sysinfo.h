#ifndef SYSINFO_H
#define SYSINFO_H

#include <stdint.h>
#include "../../bootinfo.h"

void sysinfo_init(BootInfo *info);
void sysinfo_get_cpu_brand(char *buf);
uint64_t sysinfo_get_ram_total(void);
void sysinfo_get_screen_res(uint64_t *w, uint64_t *h);

#endif

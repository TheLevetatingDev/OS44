#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include "../../bootinfo.h"

void pmm_init(BootInfo *info);
void *pmm_alloc_frame(void);
void pmm_free_frame(void *frame);

#endif

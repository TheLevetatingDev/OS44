#ifndef MEM_H
#define MEM_H

#include <stdint.h>
#include "../../bootinfo.h"

void  mem_init(BootInfo *info);
uint64_t mem_total_bytes(void);
void *mem_alloc(uint64_t size);
void *mem_alloc_aligned(uint64_t size, uint64_t align);
void  mem_free(void *ptr);

#endif

#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

void paging_init(void);
void paging_map(uint64_t vaddr, uint64_t paddr);
void paging_unmap(uint64_t vaddr);
uint64_t paging_get_physical(uint64_t vaddr);

#endif

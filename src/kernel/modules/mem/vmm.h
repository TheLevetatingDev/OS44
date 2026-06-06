#ifndef VMM_H
#define VMM_H

#include <stddef.h>
#include <stdint.h>

void vmm_init(void);
void *vmm_alloc(size_t size);
void vmm_free(void *ptr); // Placeholder, requires VMA tracking

#endif

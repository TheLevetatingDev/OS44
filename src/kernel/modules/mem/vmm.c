#include "vmm.h"
#include "paging.h"
#include "pmm.h"

// Basic VMM: allocates pages sequentially.
// This is a minimal implementation to demonstrate functionality.
// A production-grade VMM would need:
// 1. A VMA (Virtual Memory Area) list to track allocations.
// 2. A proper free mechanism to unmap pages.
// 3. Handling for page faults.

static uint64_t next_vaddr = 0x10000000; // Start at 256MB

void vmm_init(void) {
    // Nothing to do for now
}

void *vmm_alloc(size_t size) {
    size_t num_pages = (size + 4095) / 4096;
    uint64_t vaddr = next_vaddr;
    
    for (size_t i = 0; i < num_pages; i++) {
        void *paddr = pmm_alloc_frame();
        if (!paddr) return NULL; // OOM
        paging_map(vaddr + i * 4096, (uint64_t)paddr);
    }
    
    next_vaddr += num_pages * 4096;
    return (void *)vaddr;
}

void vmm_free(void *ptr) {
    // Simplistic implementation: cannot easily free without tracking VMA
    (void)ptr;
}

#include "vmm.h"
#include "paging.h"
#include "pmm.h"
#include <string.h> // For memset

typedef struct VMA {
    uint64_t vaddr;
    size_t size;
    struct VMA *next;
} VMA;

static VMA *vma_list = NULL;
static uint64_t next_vaddr = 0x10000000; // Start at 256MB

void vmm_init(void) {
    vma_list = NULL;
}

void *vmm_alloc(size_t size) {
    size_t num_pages = (size + 4095) / 4096;
    uint64_t vaddr = next_vaddr;
    
    for (size_t i = 0; i < num_pages; i++) {
        void *paddr = pmm_alloc_frame();
        if (!paddr) return NULL; // OOM
        paging_map(vaddr + i * 4096, (uint64_t)paddr);
    }
    
    // Add to VMA list (simple allocation for management structure)
    // Note: This needs a simple allocator for VMA structures themselves
    // For simplicity, we assume we have enough memory and just use static/heap space
    // A better approach is a dedicated allocator.
    static VMA vma_nodes[128]; // Preallocated for simplicity
    static int vma_count = 0;
    
    if (vma_count < 128) {
        VMA *new_vma = &vma_nodes[vma_count++];
        new_vma->vaddr = vaddr;
        new_vma->size = num_pages * 4096;
        new_vma->next = vma_list;
        vma_list = new_vma;
    }
    
    next_vaddr += num_pages * 4096;
    return (void *)vaddr;
}

void vmm_free(void *ptr) {
    VMA *prev = NULL;
    VMA *curr = vma_list;
    
    while (curr) {
        if (curr->vaddr == (uint64_t)ptr) {
            // Unmap and free frames
            size_t num_pages = curr->size / 4096;
            for (size_t i = 0; i < num_pages; i++) {
                uint64_t vaddr = curr->vaddr + i * 4096;
                uint64_t paddr = paging_get_physical(vaddr);
                if (paddr) {
                    paging_unmap(vaddr);
                    pmm_free_frame((void *)(paddr & ~0xFFF));
                }
            }
            
            // Remove from list
            if (prev) prev->next = curr->next;
            else vma_list = curr->next;
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

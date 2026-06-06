#include "mem.h"
#include "pmm.h"
#include "paging.h"
#include <stddef.h>

#define PAGE_SIZE 4096
#define EFI_CONVENTIONAL_MEMORY 7

typedef struct {
    uint64_t size;
} MemHeader;

static uint64_t total_ram_bytes = 0;

typedef struct {
    uint32_t type;
    uint32_t pad;
    uint64_t physical_start;
    uint64_t virtual_start;
    uint64_t number_of_pages;
    uint64_t attribute;
} EfiMemoryDescriptor;

void mem_init(BootInfo *info) {
    pmm_init(info);
    paging_init();

    // Calculate total RAM from memory map
    total_ram_bytes = 0;

    if (!info || !info->mmap || info->mmap_desc_size < sizeof(EfiMemoryDescriptor))
        return;

    uint64_t desc_size = info->mmap_desc_size;
    uint64_t count = info->mmap_size / desc_size;
    uint8_t *cursor = (uint8_t *)info->mmap;

    for (uint64_t i = 0; i < count; i++) {
        EfiMemoryDescriptor *desc = (EfiMemoryDescriptor *)(cursor + i * desc_size);
        // Only count conventional memory as usable/total RAM
        if (desc->type == EFI_CONVENTIONAL_MEMORY) {
            total_ram_bytes += desc->number_of_pages * PAGE_SIZE;
        }
    }
}

uint64_t mem_total_bytes(void) {
    return total_ram_bytes;
}

void *mem_alloc(uint64_t size) {
    uint64_t total_size = size + sizeof(MemHeader);
    uint64_t pages = (total_size + PAGE_SIZE - 1) / PAGE_SIZE;

    // Allocate virtual pages (simplified: just bump allocation of virtual address)
    // and map them to physical frames.
    static uint64_t next_vaddr = 0x10000000; // Start somewhere high
    void *ptr = (void *)next_vaddr;

    for (uint64_t i = 0; i < pages; i++) {
        void *pframe = pmm_alloc_frame();
        if (!pframe) return NULL; // Handle OOM
        paging_map(next_vaddr + i * PAGE_SIZE, (uint64_t)pframe);
    }
    
    MemHeader *header = (MemHeader *)ptr;
    header->size = pages;
    
    next_vaddr += pages * PAGE_SIZE;
    return (void *)(header + 1);
}

void mem_free(void *ptr) {
    if (!ptr) return;
    MemHeader *header = (MemHeader *)ptr - 1;
    uint64_t pages = header->size;
    uint64_t vaddr = (uint64_t)header;

    for (uint64_t i = 0; i < pages; i++) {
        uint64_t current_vaddr = vaddr + i * PAGE_SIZE;
        uint64_t paddr = paging_get_physical(current_vaddr);
        if (paddr) {
            paging_unmap(current_vaddr);
            pmm_free_frame((void *)paddr);
        }
    }
}

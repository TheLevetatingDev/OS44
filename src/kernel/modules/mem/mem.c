#include "mem.h"
#include "pmm.h"
#include "paging.h"
#include "../framebuffer/framebuffer.h"
#include <stddef.h>
#include <stdint.h>

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

    // With identity mapping, just find physical frames
    void *pframe = pmm_alloc_frame();
    if (!pframe) return NULL;
    
    MemHeader *header = (MemHeader *)pframe;
    header->size = pages;
    
    // Allocate remaining pages
    for (uint64_t i = 1; i < pages; i++) {
        pmm_alloc_frame(); 
    }
    
    return (void *)(header + 1);
}

void mem_free(void *ptr) {
    if (!ptr) return;
    MemHeader *header = (MemHeader *)ptr - 1;
    uint64_t pages = header->size;
    
    // Free the physical frames
    uint8_t *pframe = (uint8_t *)header;
    for (uint64_t i = 0; i < pages; i++) {
        pmm_free_frame((void *)(pframe + i * PAGE_SIZE));
    }
}

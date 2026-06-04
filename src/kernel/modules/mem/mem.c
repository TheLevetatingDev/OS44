#include "mem.h"

#define EFI_CONVENTIONAL_MEMORY 7
#define PAGE_SIZE               4096
#define HEAP_SIZE               (4 * 1024 * 1024)

typedef struct {
    uint32_t type;
    uint32_t pad;
    uint64_t physical_start;
    uint64_t virtual_start;
    uint64_t number_of_pages;
    uint64_t attribute;
} EfiMemoryDescriptor;

static uint64_t total_ram_bytes;
static uint8_t *heap_base;
static uint8_t *heap_end;
static uint8_t *heap_ptr;

static int is_conventional(const EfiMemoryDescriptor *desc) {
    return desc->type == EFI_CONVENTIONAL_MEMORY;
}

void mem_init(BootInfo *info) {
    total_ram_bytes = 0;
    heap_base = 0;
    heap_end  = 0;
    heap_ptr  = 0;

    if (!info || !info->mmap || info->mmap_desc_size < sizeof(EfiMemoryDescriptor))
        return;

    uint64_t desc_size = info->mmap_desc_size;
    uint64_t count = info->mmap_size / desc_size;
    uint8_t *cursor = (uint8_t *)info->mmap;

    EfiMemoryDescriptor *best = 0;
    uint64_t best_bytes = 0;

    for (uint64_t i = 0; i < count; i++) {
        EfiMemoryDescriptor *desc = (EfiMemoryDescriptor *)(cursor + i * desc_size);
        if (!is_conventional(desc)) continue;

        uint64_t bytes = desc->number_of_pages * PAGE_SIZE;
        total_ram_bytes += bytes;

        if (bytes > best_bytes) {
            best_bytes = bytes;
            best = desc;
        }
    }

    if (!best || best_bytes < HEAP_SIZE + PAGE_SIZE) return;

    uint64_t region_end = best->physical_start + best->number_of_pages * PAGE_SIZE;
    heap_end  = (uint8_t *)region_end;
    heap_base = heap_end - HEAP_SIZE;
    heap_ptr  = heap_base;
}

uint64_t mem_total_bytes(void) {
    return total_ram_bytes;
}

static uint8_t *align_up(uint8_t *ptr, uint64_t align) {
    uint64_t addr = (uint64_t)ptr;
    return (uint8_t *)((addr + align - 1) & ~(align - 1));
}

void *mem_alloc_aligned(uint64_t size, uint64_t align) {
    if (!heap_ptr || size == 0) return 0;
    if (align < 16) align = 16;

    uint8_t *next = align_up(heap_ptr, align);
    if (next + size > heap_end) return 0;

    void *result = next;
    heap_ptr = next + size;
    return result;
}

void *mem_alloc(uint64_t size) {
    return mem_alloc_aligned(size, 16);
}

#include "pmm.h"
#include <stddef.h>

#define PAGE_SIZE 4096
#define MAX_MEM (4ULL * 1024 * 1024 * 1024) // Assume 4GB for now
#define BITMAP_SIZE (MAX_MEM / PAGE_SIZE / 8)

static uint8_t pmm_bitmap[BITMAP_SIZE];
static uint64_t total_frames;

typedef struct {
    uint32_t type;
    uint32_t pad;
    uint64_t physical_start;
    uint64_t virtual_start;
    uint64_t number_of_pages;
    uint64_t attribute;
} EfiMemoryDescriptor;

#define EFI_CONVENTIONAL_MEMORY 7

// Helper to set/get bit
static void set_bit(uint64_t frame_idx) {
    if (frame_idx < (MAX_MEM / PAGE_SIZE))
        pmm_bitmap[frame_idx / 8] |= (1 << (frame_idx % 8));
}

static void clear_bit(uint64_t frame_idx) {
    if (frame_idx < (MAX_MEM / PAGE_SIZE))
        pmm_bitmap[frame_idx / 8] &= ~(1 << (frame_idx % 8));
}

static int test_bit(uint64_t frame_idx) {
    if (frame_idx >= (MAX_MEM / PAGE_SIZE)) return 1; // Mark out of bounds as used
    return (pmm_bitmap[frame_idx / 8] & (1 << (frame_idx % 8))) != 0;
}

void pmm_init(BootInfo *info) {
    total_frames = MAX_MEM / PAGE_SIZE;

    // 1. Initialize bitmap to all used (1)
    for (uint64_t i = 0; i < BITMAP_SIZE; i++) pmm_bitmap[i] = 0xFF;

    // 2. Scan mmap and mark conventional memory as free (0)
    uint64_t desc_size = info->mmap_desc_size;
    uint64_t count = info->mmap_size / desc_size;
    uint8_t *cursor = (uint8_t *)info->mmap;

    for (uint64_t i = 0; i < count; i++) {
        EfiMemoryDescriptor *desc = (EfiMemoryDescriptor *)(cursor + i * desc_size);
        if (desc->type == EFI_CONVENTIONAL_MEMORY) {
            uint64_t start_frame = desc->physical_start / PAGE_SIZE;
            for (uint64_t f = 0; f < desc->number_of_pages; f++) {
                clear_bit(start_frame + f);
            }
        }
    }
    
    // 3. Mark kernel-occupied frames as used (1) - Placeholder: 
    // Ideally should read symbol table or linker script to find kernel bounds.
    // Mark first 8MB as used to cover kernel, bitmap, and buffers
    for (uint64_t f = 0; f < (8 * 1024 * 1024 / PAGE_SIZE); f++) {
        set_bit(f);
    }
}

void *pmm_alloc_frame(void) {
    for (uint64_t i = 0; i < total_frames; i++) {
        if (!test_bit(i)) {
            set_bit(i);
            return (void *)(i * PAGE_SIZE);
        }
    }
    return NULL;
}

void pmm_free_frame(void *frame) {
    uint64_t frame_idx = (uint64_t)frame / PAGE_SIZE;
    clear_bit(frame_idx);
}

uint64_t pmm_get_free_frames(void) {
    uint64_t free = 0;
    for (uint64_t i = 0; i < total_frames; i++) {
        if (!test_bit(i)) free++;
    }
    return free;
}

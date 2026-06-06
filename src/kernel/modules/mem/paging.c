#include "paging.h"
#include "pmm.h"
#include <stddef.h>

#define PAGE_PRESENT (1ULL << 0)
#define PAGE_WRITABLE (1ULL << 1)

// Basic Page Table structure (simplistic for now)
typedef uint64_t PageTableEntry;
typedef struct {
    PageTableEntry entries[512];
} PageTable;

static PageTable *pml4 = NULL;

// In a real OS, we'd need to find the current CR3. 
// Assuming identity mapping, we can try to find it.
void paging_init(void) {
    // For now, we assume we can traverse from a known base
    // In reality, this needs to be set up carefully.
}

static PageTable *get_next_table(PageTable *table, uint64_t index, int create) {
    if (!(table->entries[index] & PAGE_PRESENT)) {
        if (!create) return NULL;
        void *new_frame = pmm_alloc_frame();
        if (!new_frame) return NULL;
        
        table->entries[index] = (uint64_t)new_frame | PAGE_PRESENT | PAGE_WRITABLE;
        // In a real setup, need to clear the new table
    }
    return (PageTable *)(table->entries[index] & ~0xFFF);
}

void paging_map(uint64_t vaddr, uint64_t paddr) {
    // Traverse PML4 -> PDPT -> PD -> PT, create if missing, set entry
}

void paging_unmap(uint64_t vaddr) {
    // Traverse to PT, clear Present bit, invalidate TLB
}

uint64_t paging_get_physical(uint64_t vaddr) {
    // Traverse tables to find physical frame
    return 0; // Placeholder
}

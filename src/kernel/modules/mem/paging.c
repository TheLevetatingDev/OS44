#include "paging.h"
#include "pmm.h"
#include <stddef.h>
#include <string.h>

#define PAGE_PRESENT (1ULL << 0)
#define PAGE_WRITABLE (1ULL << 1)
#define PAGE_SIZE 4096

typedef uint64_t PageTableEntry;
typedef struct {
    PageTableEntry entries[512];
} PageTable;

static PageTable *pml4 = NULL;

void paging_init(void) {
    // For now, we assume identity mapping, and try to get CR3
    uint64_t cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r" (cr3));
    pml4 = (PageTable *)(cr3 & ~0xFFF);
}

static PageTable *get_next_table(PageTable *table, uint64_t index, int create) {
    if (!(table->entries[index] & PAGE_PRESENT)) {
        if (!create) return NULL;
        void *new_frame = pmm_alloc_frame();
        if (!new_frame) return NULL;
        
        // Clear new table
        memset(new_frame, 0, PAGE_SIZE);
        
        table->entries[index] = (uint64_t)new_frame | PAGE_PRESENT | PAGE_WRITABLE;
    }
    return (PageTable *)(table->entries[index] & ~0xFFF);
}

void paging_map(uint64_t vaddr, uint64_t paddr) {
    if (!pml4) return;

    uint64_t pml4_idx = (vaddr >> 39) & 0x1FF;
    uint64_t pdpt_idx = (vaddr >> 30) & 0x1FF;
    uint64_t pd_idx   = (vaddr >> 21) & 0x1FF;
    uint64_t pt_idx   = (vaddr >> 12) & 0x1FF;

    PageTable *pdpt = get_next_table(pml4, pml4_idx, 1);
    PageTable *pd   = get_next_table(pdpt, pdpt_idx, 1);
    PageTable *pt   = get_next_table(pd,   pd_idx,   1);

    if (!pt) return;

    pt->entries[pt_idx] = (paddr & ~0xFFF) | PAGE_PRESENT | PAGE_WRITABLE;
    
    // In a real OS, need to flush TLB
    __asm__ volatile ("invlpg (%0)" :: "r" (vaddr) : "memory");
}

void paging_unmap(uint64_t vaddr) {
    if (!pml4) return;

    uint64_t pml4_idx = (vaddr >> 39) & 0x1FF;
    uint64_t pdpt_idx = (vaddr >> 30) & 0x1FF;
    uint64_t pd_idx   = (vaddr >> 21) & 0x1FF;
    uint64_t pt_idx   = (vaddr >> 12) & 0x1FF;

    PageTable *pdpt = get_next_table(pml4, pml4_idx, 0);
    if (!pdpt) return;
    PageTable *pd   = get_next_table(pdpt, pdpt_idx, 0);
    if (!pd) return;
    PageTable *pt   = get_next_table(pd,   pd_idx,   0);
    if (!pt) return;

    pt->entries[pt_idx] &= ~PAGE_PRESENT;
    
    // In a real OS, need to flush TLB
    __asm__ volatile ("invlpg (%0)" :: "r" (vaddr) : "memory");
}

uint64_t paging_get_physical(uint64_t vaddr) {
    if (!pml4) return 0;

    uint64_t pml4_idx = (vaddr >> 39) & 0x1FF;
    uint64_t pdpt_idx = (vaddr >> 30) & 0x1FF;
    uint64_t pd_idx   = (vaddr >> 21) & 0x1FF;
    uint64_t pt_idx   = (vaddr >> 12) & 0x1FF;

    PageTable *pdpt = get_next_table(pml4, pml4_idx, 0);
    if (!pdpt) return 0;
    PageTable *pd   = get_next_table(pdpt, pdpt_idx, 0);
    if (!pd) return 0;
    PageTable *pt   = get_next_table(pd,   pd_idx,   0);
    if (!pt) return 0;

    if (!(pt->entries[pt_idx] & PAGE_PRESENT)) return 0;
    
    return (pt->entries[pt_idx] & ~0xFFF) | (vaddr & 0xFFF);
}

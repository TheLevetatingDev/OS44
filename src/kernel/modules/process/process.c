#include "process.h"
#include "../mem/vmm.h"
#include "../timer/timer.h"
#include <string.h>

#define MAX_PROCS 16
static pcb_t proc_table[MAX_PROCS];
static uint64_t last_time = 0;

void process_init(void) {
    memset(proc_table, 0, sizeof(proc_table));
    last_time = timer_get_uptime_seconds();
}

uint64_t process_create(void (*entry)(void), size_t stack_size) {
    for (int i = 0; i < MAX_PROCS; i++) {
        if (proc_table[i].pid == 0) {
            // Find lowest available PID
            uint64_t lowest_pid = 1;
            int found;
            do {
                found = 0;
                for (int j = 0; j < MAX_PROCS; j++) {
                    if (proc_table[j].pid == lowest_pid) {
                        lowest_pid++;
                        found = 1;
                        break;
                    }
                }
            } while (found);
            
            proc_table[i].pid = lowest_pid;
            proc_table[i].status = PROC_RUNNING; // Set as running immediately
            proc_table[i].mem_base = vmm_alloc(stack_size);
            proc_table[i].mem_size = stack_size;
            proc_table[i].stack_ptr = (void*)((uint64_t)proc_table[i].mem_base + stack_size);
            proc_table[i].run_time = 0; // Initialize run_time
            
            // Setup minimal stack frame for entry point
            uint64_t *stack = (uint64_t *)proc_table[i].stack_ptr;
            *(--stack) = (uint64_t)entry; // Rip
            proc_table[i].stack_ptr = stack;
            
            return proc_table[i].pid;
        }
    }
    return 0; // Failed
}

void process_check_and_free(void) {
    for (int i = 0; i < MAX_PROCS; i++) {
        if (proc_table[i].pid != 0 && proc_table[i].status == PROC_TERMINATED) {
            vmm_free(proc_table[i].mem_base);
            proc_table[i].pid = 0; // Mark as unused
        }
    }
}

void scheduler(void) {
    uint64_t current_time = timer_get_uptime_seconds();
    if (current_time > last_time) {
        // Increment run_time for all running processes
        for (int i = 0; i < MAX_PROCS; i++) {
            if (proc_table[i].pid != 0 && proc_table[i].status == PROC_RUNNING) {
                proc_table[i].run_time += (current_time - last_time);
            }
        }
        last_time = current_time;
    }
}

pcb_t* get_proc_by_pid(uint64_t pid) {
    for (int i = 0; i < MAX_PROCS; i++) {
        if (proc_table[i].pid == pid) {
            return &proc_table[i];
        }
    }
    return NULL;
}

void kill_proc(uint64_t pid) {
    pcb_t *proc = get_proc_by_pid(pid);
    if (proc) {
        proc->status = PROC_TERMINATED;
    }
}

#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    PROC_READY,
    PROC_RUNNING,
    PROC_TERMINATED
} proc_status_t;

typedef struct {
    uint64_t pid;
    proc_status_t status;
    void *stack_ptr;
    // For simplicity, we just track the base of the allocated memory for now
    void *mem_base;
    size_t mem_size;
    uint64_t run_time; // Added run_time
} pcb_t;

void process_init(void);
uint64_t process_create(void (*entry)(void), size_t stack_size);
void process_check_and_free(void);
void scheduler(void);
pcb_t* get_proc_by_pid(uint64_t pid);
void kill_proc(uint64_t pid);

#endif

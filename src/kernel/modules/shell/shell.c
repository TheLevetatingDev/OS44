#include "shell.h"
#include "../framebuffer/framebuffer.h"
#include "../../string.h"
#include "../mem/pmm.h"
#include "../mem/paging.h"
#include "../mem/vmm.h"
#include "../process/process.h"
#include "../sysinfo/sysinfo.h"
#include <stdint.h>

// Helper to convert int to string
static void itoa(uint64_t val, char *buf) {
    char *p = buf;
    if (val == 0) {
        *p++ = '0';
        *p = '\0';
        return;
    }
    char tmp[20];
    int i = 0;
    while (val > 0) {
        tmp[i++] = (val % 10) + '0';
        val /= 10;
    }
    while (i > 0) *p++ = tmp[--i];
    *p = '\0';
}

static uint64_t atoi(const char *str) {
    uint64_t res = 0;
    while (*str >= '0' && *str <= '9') {
        res = res * 10 + (*str - '0');
        str++;
    }
    return res;
}

#define MAX_CMD_LEN 64
#define MAX_LINES 20
#define LINE_LEN 80

static char cmd_buf[MAX_CMD_LEN];
static uint64_t cmd_pos = 0;
static char terminal_history[MAX_LINES][LINE_LEN];
static uint64_t history_lines = 0;
static uint64_t prompt_y = 16;

static void add_to_history(const char *str) {
    if (history_lines < MAX_LINES) {
        memcpy(terminal_history[history_lines++], str, LINE_LEN);
    } else {
        for (uint64_t i = 0; i < MAX_LINES - 1; i++) {
            memcpy(terminal_history[i], terminal_history[i + 1], LINE_LEN);
        }
        memcpy(terminal_history[MAX_LINES - 1], str, LINE_LEN);
    }
}

static int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// Dummy entry point for test process
void test_entry(void) { while(1); }

static void execute_command(void) {
    if (cmd_pos == 0) return;
    cmd_buf[cmd_pos] = '\0';
    
    char full_prompt[MAX_CMD_LEN + 15] = "OS44 Shell> ";
    memcpy(full_prompt + 12, cmd_buf, cmd_pos);
    full_prompt[12 + cmd_pos] = '\0';
    add_to_history(full_prompt);
    
    char response[LINE_LEN];

    if (strcmp(cmd_buf, "help") == 0) {
        memcpy(response, "Cmds: help, testmem, spawn, get [pid], pkill [pid], info", 60);
    } else if (strcmp(cmd_buf, "info") == 0) {
        char sysinfo_buf[512];
        sysinfo_get_sysinfo_string(sysinfo_buf);
        add_to_history("System Information:");
        
        char *ptr = sysinfo_buf;
        char *start = sysinfo_buf;
        while (*ptr != '\0') {
            if (*ptr == '|') {
                *ptr = '\0';
                add_to_history(start);
                start = ptr + 1;
            }
            ptr++;
        }
        add_to_history(start); // Add last segment
        
        memcpy(response, "Info retrieved", 14);
    } else if (strcmp(cmd_buf, "testmem") == 0) {
        uint64_t free_frames = pmm_get_free_frames();
        char frames_str[20];
        itoa(free_frames, frames_str);
        
        char msg[LINE_LEN];
        memcpy(msg, "Free frames: ", 13);
        memcpy(msg + 13, frames_str, 20);
        add_to_history(msg);
        
        // VMM Test
        void *ptr = vmm_alloc(8192); // 2 pages
        if (ptr) {
            add_to_history("VMM test: Success");
            vmm_free(ptr); // Free the memory
        } else {
            add_to_history("VMM test: Failed");
        }
        
        memcpy(response, "Testmem complete", 16);
    } else if (strcmp(cmd_buf, "spawn") == 0) {
        uint64_t pid = process_create(test_entry, 4096);
        char pid_str[20];
        itoa(pid, pid_str);
        memcpy(response, "Spawned PID: ", 13);
        memcpy(response + 13, pid_str, 20);
    } else if (cmd_buf[0] == 'g' && cmd_buf[1] == 'e' && cmd_buf[2] == 't') {
        uint64_t pid = atoi(cmd_buf + 4);
        pcb_t *proc = get_proc_by_pid(pid);
        if (proc) {
            char mem_str[20];
            itoa(proc->mem_size, mem_str);
            char time_str[20];
            itoa(proc->run_time, time_str);
            
            char status_str[20];
            memcpy(status_str, (proc->status == PROC_RUNNING ? "RUNNING" : "READY"), 7);
            status_str[7] = '\0';
            
            char msg[LINE_LEN];
            memcpy(msg, "PID status: ", 12);
            memcpy(msg + 12, status_str, 8);
            add_to_history(msg);
            
            memcpy(msg, "Mem: ", 5);
            memcpy(msg + 5, mem_str, 20);
            add_to_history(msg);
            
            memcpy(response, "Time: ", 6);
            memcpy(response + 6, time_str, 20);
        } else {
            memcpy(response, "Invalid PID", 11);
        }
    } else if (cmd_buf[0] == 'p' && cmd_buf[1] == 'k' && cmd_buf[2] == 'i' && cmd_buf[3] == 'l' && cmd_buf[4] == 'l') {
        uint64_t pid = atoi(cmd_buf + 6);
        pcb_t *proc = get_proc_by_pid(pid);
        if (proc) {
            kill_proc(pid);
            process_check_and_free(); // Immediately clean up
            memcpy(response, "Killed PID", 10);
        } else {
            memcpy(response, "Invalid PID", 11);
        }
    } else {
        memcpy(response, "Unknown command", 15);
    }
    add_to_history(response);
    
    cmd_pos = 0;
    shell_render();
}

void shell_init(void) {
    cmd_pos = 0;
    history_lines = 0;
    prompt_y = 16;
}

void shell_putc(char c) {
    if (c == '\b') {
        if (cmd_pos > 0) {
            cmd_pos--;
            char space[] = " ";
            fb_draw_string(space, 32 + 12*8 + cmd_pos*8, prompt_y, FB_COLOR_WHITE, FB_COLOR_BLACK);
        }
    } else if (c == '\n' || c == '\r') {
        execute_command();
    } else if (cmd_pos < MAX_CMD_LEN - 1) {
        cmd_buf[cmd_pos++] = c;
        char str[2] = {c, '\0'};
        fb_draw_string(str, 32 + 12*8 + (cmd_pos - 1)*8, prompt_y, FB_COLOR_GREEN, FB_COLOR_BLACK);
    }
}

void shell_render(void) {
    fb_clear(FB_COLOR_BLACK);
    
    uint64_t y = 16;
    for (uint64_t i = 0; i < history_lines; i++) {
        fb_draw_string(terminal_history[i], 32, y, FB_COLOR_WHITE, FB_COLOR_BLACK);
        y += 16;
    }
    
    prompt_y = y;
    
    char prompt[MAX_CMD_LEN + 15] = "OS44 Shell> ";
    memcpy(prompt + 12, cmd_buf, cmd_pos);
    prompt[12 + cmd_pos] = '\0';
    fb_draw_string(prompt, 32, y, FB_COLOR_GREEN, FB_COLOR_BLACK);
}

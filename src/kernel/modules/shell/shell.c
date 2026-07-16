#include "shell.h"
#include "../framebuffer/framebuffer.h"
#include "../../string.h"
#include "../mem/pmm.h"
#include "../mem/paging.h"
#include "../mem/vmm.h"
#include "../process/process.h"
#include "../sysinfo/sysinfo.h"
#include "../ide/ide.h"
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

static void append_str(char *buf, uint64_t *pos, uint64_t cap, const char *s) {
    while (*s && *pos + 1 < cap)
        buf[(*pos)++] = *s++;
}

static void append_hex(char *buf, uint64_t *pos, uint64_t cap, uint64_t value) {
    static const char hex[] = "0123456789ABCDEF";
    char tmp[18];
    int len = 0;
    if (value == 0) {
        if (*pos + 3 < cap) {
            buf[(*pos)++] = '0';
            buf[(*pos)++] = 'x';
            buf[(*pos)++] = '0';
        }
        return;
    }
    while (value > 0 && len < 16) {
        tmp[len++] = hex[value & 0xF];
        value >>= 4;
    }
    if (*pos + 2 < cap) {
        buf[(*pos)++] = '0';
        buf[(*pos)++] = 'x';
    }
    while (len > 0 && *pos + 1 < cap)
        buf[(*pos)++] = tmp[--len];
}

static void append_dec(char *buf, uint64_t *pos, uint64_t cap, uint64_t value) {
    char tmp[24];
    int len = 0;
    if (value == 0) {
        if (*pos + 1 < cap) buf[(*pos)++] = '0';
        return;
    }
    while (value > 0 && len < 24) {
        tmp[len++] = (char)('0' + (value % 10));
        value /= 10;
    }
    while (len > 0 && *pos + 1 < cap)
        buf[(*pos)++] = tmp[--len];
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
        memcpy(response, "Cmds: help, testmem, spawn, get [pid], pkill [pid], info, ideinfo, ideread [lba] [count], ideident", 100);
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
    } else if (strcmp(cmd_buf, "ideinfo") == 0) {
        char msg[256];
        char *p = msg;
        size_t remaining = sizeof(msg);
        
        extern uint8_t ide_primary_master;
        extern uint8_t ide_primary_slave;
        extern uint8_t ide_secondary_master;
        extern uint8_t ide_secondary_slave;
        extern uint32_t ide_primary_master_sectors;
        extern uint32_t ide_primary_slave_sectors;
        extern uint32_t ide_secondary_master_sectors;
        extern uint32_t ide_secondary_slave_sectors;
        
        append_str(msg, (uint64_t*)&p, remaining, "IDE Drive Info:\n");
        add_to_history(msg);
        
        p = msg; remaining = sizeof(msg);
        if (ide_primary_master) {
            append_str(msg, (uint64_t*)&p, remaining, "  Primary Master: Present, Sectors: ");
            append_dec(msg, (uint64_t*)&p, remaining, ide_primary_master_sectors);
        } else {
            append_str(msg, (uint64_t*)&p, remaining, "  Primary Master: Not present");
        }
        add_to_history(msg);
        
        p = msg; remaining = sizeof(msg);
        if (ide_primary_slave) {
            append_str(msg, (uint64_t*)&p, remaining, "  Primary Slave: Present, Sectors: ");
            append_dec(msg, (uint64_t*)&p, remaining, ide_primary_slave_sectors);
        } else {
            append_str(msg, (uint64_t*)&p, remaining, "  Primary Slave: Not present");
        }
        add_to_history(msg);
        
        p = msg; remaining = sizeof(msg);
        if (ide_secondary_master) {
            append_str(msg, (uint64_t*)&p, remaining, "  Secondary Master: Present, Sectors: ");
            append_dec(msg, (uint64_t*)&p, remaining, ide_secondary_master_sectors);
        } else {
            append_str(msg, (uint64_t*)&p, remaining, "  Secondary Master: Not present");
        }
        add_to_history(msg);
        
        p = msg; remaining = sizeof(msg);
        if (ide_secondary_slave) {
            append_str(msg, (uint64_t*)&p, remaining, "  Secondary Slave: Present, Sectors: ");
            append_dec(msg, (uint64_t*)&p, remaining, ide_secondary_slave_sectors);
        } else {
            append_str(msg, (uint64_t*)&p, remaining, "  Secondary Slave: Not present");
        }
        add_to_history(msg);
        
        memcpy(response, "IDE info printed", 16);
    } else if (cmd_buf[0] == 'i' && cmd_buf[1] == 'd' && cmd_buf[2] == 'e' && cmd_buf[3] == 'r' && cmd_buf[4] == 'e' && cmd_buf[5] == 'a' && cmd_buf[6] == 'd') {
        uint32_t lba = atoi(cmd_buf + 8);
        uint16_t count = 1;
        char *space = (char*)cmd_buf + 8;
        while (*space && *space != ' ') space++;
        if (*space == ' ') count = atoi(space + 1);
        
        uint8_t buf[512 * 16];
        if (count > 16) count = 16;
        int ret = ide_read_sectors(0, lba, count, buf);
        if (ret == 0) {
            char line[128];
            for (uint16_t i = 0; i < count * 512; i += 16) {
                uint64_t pos = 0;
                append_str(line, &pos, sizeof(line), "LBA+");
                append_hex(line, &pos, sizeof(line), lba + i / 512);
                append_str(line, &pos, sizeof(line), " +");
                append_dec(line, &pos, sizeof(line), i / 512);
                append_str(line, &pos, sizeof(line), ": ");
                for (int j = 0; j < 16 && i + j < count * 512; j++) {
                    append_hex(line, &pos, sizeof(line), buf[i + j]);
                    append_str(line, &pos, sizeof(line), " ");
                }
                line[pos < sizeof(line) ? pos : sizeof(line) - 1] = '\0';
                add_to_history(line);
            }
            memcpy(response, "Read complete", 13);
        } else {
            memcpy(response, "Read failed", 11);
        }
    } else if (strcmp(cmd_buf, "ideident") == 0) {
        ide_identify_t identify;
        ide_identify(0, &identify);
        
        char msg[256];
        char *p = msg;
        size_t remaining = sizeof(msg);
        
        append_str(msg, (uint64_t*)&p, remaining, "IDENTIFY for drive 0:\n");
        add_to_history(msg);
        
        if (identify.config == 0) {
            add_to_history("  Drive not present or IDENTIFY failed");
        } else {
            char model[41];
            memcpy(model, identify.model_number, 40);
            model[40] = 0;
            
            p = msg; remaining = sizeof(msg);
            append_str(msg, (uint64_t*)&p, remaining, "  Model: ");
            append_str(msg, (uint64_t*)&p, remaining, model);
            append_str(msg, (uint64_t*)&p, remaining, "\n");
            add_to_history(msg);
            
            char serial[21];
            memcpy(serial, identify.serial_number, 20);
            serial[20] = 0;
            
            p = msg; remaining = sizeof(msg);
            append_str(msg, (uint64_t*)&p, remaining, "  Serial: ");
            append_str(msg, (uint64_t*)&p, remaining, serial);
            append_str(msg, (uint64_t*)&p, remaining, "\n");
            add_to_history(msg);
            
            char fw[9];
            memcpy(fw, identify.firmware_revision, 8);
            fw[8] = 0;
            
            p = msg; remaining = sizeof(msg);
            append_str(msg, (uint64_t*)&p, remaining, "  Firmware: ");
            append_str(msg, (uint64_t*)&p, remaining, fw);
            append_str(msg, (uint64_t*)&p, remaining, "\n");
            add_to_history(msg);
            
            p = msg; remaining = sizeof(msg);
            append_str(msg, (uint64_t*)&p, remaining, "  Cylinders: ");
            append_dec(msg, (uint64_t*)&p, remaining, identify.cylinders);
            append_str(msg, (uint64_t*)&p, remaining, "\n");
            add_to_history(msg);
            
            p = msg; remaining = sizeof(msg);
            append_str(msg, (uint64_t*)&p, remaining, "  Heads: ");
            append_dec(msg, (uint64_t*)&p, remaining, identify.heads);
            append_str(msg, (uint64_t*)&p, remaining, "\n");
            add_to_history(msg);
            
            p = msg; remaining = sizeof(msg);
            append_str(msg, (uint64_t*)&p, remaining, "  Sectors/Track: ");
            append_dec(msg, (uint64_t*)&p, remaining, identify.sectors_per_track);
            append_str(msg, (uint64_t*)&p, remaining, "\n");
            add_to_history(msg);
            
            p = msg; remaining = sizeof(msg);
            append_str(msg, (uint64_t*)&p, remaining, "  Total LBA28 sectors: ");
            append_dec(msg, (uint64_t*)&p, remaining, identify.total_lba28);
            append_str(msg, (uint64_t*)&p, remaining, "\n");
            add_to_history(msg);
            
            p = msg; remaining = sizeof(msg);
            append_str(msg, (uint64_t*)&p, remaining, "  Capacity (MB): ");
            append_dec(msg, (uint64_t*)&p, remaining, (identify.total_lba28 * 512) / (1024 * 1024));
            append_str(msg, (uint64_t*)&p, remaining, "\n");
            add_to_history(msg);
        }
        memcpy(response, "IDENTIFY sent", 13);
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

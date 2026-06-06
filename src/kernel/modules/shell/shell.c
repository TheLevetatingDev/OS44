#include "shell.h"
#include "../framebuffer/framebuffer.h"
#include "../../string.h"
#include <stdint.h>

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

static void execute_command(void) {
    if (cmd_pos == 0) return;
    cmd_buf[cmd_pos] = '\0';
    
    char full_prompt[MAX_CMD_LEN + 15] = "OS44 Shell> ";
    memcpy(full_prompt + 12, cmd_buf, cmd_pos);
    full_prompt[12 + cmd_pos] = '\0';
    add_to_history(full_prompt);
    
    char response[LINE_LEN];
    if (strcmp(cmd_buf, "help") == 0) {
        memcpy(response, "Commands: help, test", 20);
    } else if (strcmp(cmd_buf, "test") == 0) {
        memcpy(response, "Test: Interrupts functional", 27);
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

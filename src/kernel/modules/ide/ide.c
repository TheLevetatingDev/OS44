#include "ide.h"
#include "../interrupts/interrupts.h"
#include "../framebuffer/framebuffer.h"
#include <stdint.h>
#include <string.h>

extern void add_to_history(const char *str);

static void append_str(char *buf, uint64_t *pos, uint64_t cap, const char *s) {
    while (*s && *pos + 1 < cap)
        buf[(*pos)++] = *s++;
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

volatile int ide_irq_fired = 0;
volatile int ide_last_status = 0;
volatile int ide_last_error = 0;

uint8_t ide_primary_master = 0;
uint8_t ide_primary_slave = 0;
uint8_t ide_secondary_master = 0;
uint8_t ide_secondary_slave = 0;

uint32_t ide_primary_master_sectors = 0;
uint32_t ide_primary_slave_sectors = 0;
uint32_t ide_secondary_master_sectors = 0;
uint32_t ide_secondary_slave_sectors = 0;

static void ide_select_drive(uint16_t base, uint8_t drive) {
    outb(base + 6, 0xE0 | (drive << 4));
}

static void ide_wait_400ns(uint16_t base) {
    for (int i = 0; i < 4; i++) {
        inb(base + 7);
    }
}

static int ide_poll(uint16_t base, uint32_t timeout, int advanced_check) {
    uint32_t count = 0;
    while (count < timeout) {
        uint8_t status = inb(base + 7);
        if (status & IDE_STATUS_BSY) {
            count++;
            continue;
        }
        if (advanced_check) {
            if (status & IDE_STATUS_ERR) {
                ide_last_error = inb(base + 1);
                ide_last_status = status;
                return -1;
            }
            if (status & IDE_STATUS_DRQ) {
                ide_last_status = status;
                return 0;
            }
        } else {
            ide_last_status = status;
            if (status & IDE_STATUS_DRQ) return 0;
            if (status & IDE_STATUS_ERR) {
                ide_last_error = inb(base + 1);
                return -1;
            }
            return 0;
        }
        count++;
    }
    return -1;
}

static int ide_wait_ready_impl(uint16_t base, uint32_t timeout) {
    return ide_poll(base, timeout, 1);
}

void ide_init(void) {
    fb_draw_string("IDE: Initializing IDE controller...", 16, 256, FB_COLOR_WHITE, FB_COLOR_BLACK);
    
    outb(IDE_PRIMARY_CTRL + 2, 0x02);
    outb(IDE_SECONDARY_CTRL + 2, 0x02);
    
    ide_identify_t identify;
    
    ide_select_drive(IDE_PRIMARY_BASE, 0);
    outb(IDE_PRIMARY_BASE + 2, 0);
    outb(IDE_PRIMARY_BASE + 3, 0);
    outb(IDE_PRIMARY_BASE + 4, 0);
    outb(IDE_PRIMARY_BASE + 5, 0);
    outb(IDE_PRIMARY_BASE + 7, IDE_CMD_IDENTIFY);
    
    uint8_t status = inb(IDE_PRIMARY_BASE + 7);
    if (status) {
        ide_wait_ready_impl(IDE_PRIMARY_BASE, 100000);
        uint16_t *buf = (uint16_t*)&identify;
        for (int i = 0; i < 256; i++) {
            buf[i] = inw(IDE_PRIMARY_BASE);
        }
        ide_primary_master = 1;
        ide_primary_master_sectors = identify.total_lba28;
        fb_draw_string("IDE: Primary Master detected", 16, 272, FB_COLOR_GREEN, FB_COLOR_BLACK);
        
        char model[41];
        memcpy(model, identify.model_number, 40);
        model[40] = 0;
        char msg[80];
        char *p = msg;
        const char *s = "IDE: Model: ";
        while (*s) *p++ = *s++;
        s = model;
        while (*s && p < msg + 79) *p++ = *s++;
        *p = 0;
        fb_draw_string(msg, 16, 288, FB_COLOR_WHITE, FB_COLOR_BLACK);
    } else {
        fb_draw_string("IDE: No Primary Master", 16, 272, FB_COLOR_RED, FB_COLOR_BLACK);
    }
    
    ide_select_drive(IDE_PRIMARY_BASE, 1);
    outb(IDE_PRIMARY_BASE + 2, 0);
    outb(IDE_PRIMARY_BASE + 3, 0);
    outb(IDE_PRIMARY_BASE + 4, 0);
    outb(IDE_PRIMARY_BASE + 5, 0);
    outb(IDE_PRIMARY_BASE + 7, IDE_CMD_IDENTIFY);
    
    status = inb(IDE_PRIMARY_BASE + 7);
    if (status) {
        ide_wait_ready_impl(IDE_PRIMARY_BASE, 100000);
        uint16_t *buf = (uint16_t*)&identify;
        for (int i = 0; i < 256; i++) {
            buf[i] = inw(IDE_PRIMARY_BASE);
        }
        ide_primary_slave = 1;
        ide_primary_slave_sectors = identify.total_lba28;
        fb_draw_string("IDE: Primary Slave detected", 16, 304, FB_COLOR_GREEN, FB_COLOR_BLACK);
    } else {
        fb_draw_string("IDE: No Primary Slave", 16, 304, FB_COLOR_RED, FB_COLOR_BLACK);
    }
    
    ide_select_drive(IDE_SECONDARY_BASE, 0);
    outb(IDE_SECONDARY_BASE + 2, 0);
    outb(IDE_SECONDARY_BASE + 3, 0);
    outb(IDE_SECONDARY_BASE + 4, 0);
    outb(IDE_SECONDARY_BASE + 5, 0);
    outb(IDE_SECONDARY_BASE + 7, IDE_CMD_IDENTIFY);
    
    status = inb(IDE_SECONDARY_BASE + 7);
    if (status) {
        ide_wait_ready_impl(IDE_SECONDARY_BASE, 100000);
        uint16_t *buf = (uint16_t*)&identify;
        for (int i = 0; i < 256; i++) {
            buf[i] = inw(IDE_SECONDARY_BASE);
        }
        ide_secondary_master = 1;
        ide_secondary_master_sectors = identify.total_lba28;
        fb_draw_string("IDE: Secondary Master detected", 16, 320, FB_COLOR_GREEN, FB_COLOR_BLACK);
    } else {
        fb_draw_string("IDE: No Secondary Master", 16, 320, FB_COLOR_RED, FB_COLOR_BLACK);
    }
    
    ide_select_drive(IDE_SECONDARY_BASE, 1);
    outb(IDE_SECONDARY_BASE + 2, 0);
    outb(IDE_SECONDARY_BASE + 3, 0);
    outb(IDE_SECONDARY_BASE + 4, 0);
    outb(IDE_SECONDARY_BASE + 5, 0);
    outb(IDE_SECONDARY_BASE + 7, IDE_CMD_IDENTIFY);
    
    status = inb(IDE_SECONDARY_BASE + 7);
    if (status) {
        ide_wait_ready_impl(IDE_SECONDARY_BASE, 100000);
        uint16_t *buf = (uint16_t*)&identify;
        for (int i = 0; i < 256; i++) {
            buf[i] = inw(IDE_SECONDARY_BASE);
        }
        ide_secondary_slave = 1;
        ide_secondary_slave_sectors = identify.total_lba28;
        fb_draw_string("IDE: Secondary Slave detected", 16, 336, FB_COLOR_GREEN, FB_COLOR_BLACK);
    } else {
        fb_draw_string("IDE: No Secondary Slave", 16, 336, FB_COLOR_RED, FB_COLOR_BLACK);
    }
    
    fb_draw_string("IDE: Initialization complete", 16, 352, FB_COLOR_GREEN, FB_COLOR_BLACK);
}

int ide_detect_drives(void) {
    int count = 0;
    if (ide_primary_master) count++;
    if (ide_primary_slave) count++;
    if (ide_secondary_master) count++;
    if (ide_secondary_slave) count++;
    return count;
}

void ide_identify(uint8_t drive, ide_identify_t *identify) {
    uint16_t base;
    uint8_t drive_sel;
    
    if (drive == 0) { base = IDE_PRIMARY_BASE; drive_sel = 0; }
    else if (drive == 1) { base = IDE_PRIMARY_BASE; drive_sel = 1; }
    else if (drive == 2) { base = IDE_SECONDARY_BASE; drive_sel = 0; }
    else { base = IDE_SECONDARY_BASE; drive_sel = 1; }
    
    ide_select_drive(base, drive_sel);
    outb(base + 2, 0);
    outb(base + 3, 0);
    outb(base + 4, 0);
    outb(base + 5, 0);
    outb(base + 7, IDE_CMD_IDENTIFY);
    
    uint8_t status = inb(base + 7);
    if (!status) {
        memset(identify, 0, sizeof(ide_identify_t));
        return;
    }
    
    ide_wait_ready_impl(base, 100000);
    uint16_t *buf = (uint16_t*)identify;
    for (int i = 0; i < 256; i++) {
        buf[i] = inw(base);
    }
}

static int ide_read_write_sectors(uint8_t drive, uint32_t lba, uint16_t count, void *buffer, int write) {
    if (count == 0) return 0;
    
    uint16_t base;
    uint8_t drive_sel;
    
    if (drive == 0) { base = IDE_PRIMARY_BASE; drive_sel = 0; }
    else if (drive == 1) { base = IDE_PRIMARY_BASE; drive_sel = 1; }
    else if (drive == 2) { base = IDE_SECONDARY_BASE; drive_sel = 0; }
    else { base = IDE_SECONDARY_BASE; drive_sel = 1; }
    
    if (write) {
        if (drive == 0 && !ide_primary_master) return -1;
        if (drive == 1 && !ide_primary_slave) return -1;
        if (drive == 2 && !ide_secondary_master) return -1;
        if (drive == 3 && !ide_secondary_slave) return -1;
    } else {
        if (drive == 0 && !ide_primary_master) return -1;
        if (drive == 1 && !ide_primary_slave) return -1;
        if (drive == 2 && !ide_secondary_master) return -1;
        if (drive == 3 && !ide_secondary_slave) return -1;
    }
    
    ide_select_drive(base, drive_sel);
    
    outb(base + 1, 0);
    outb(base + 2, count);
    outb(base + 3, (uint8_t)(lba & 0xFF));
    outb(base + 4, (uint8_t)((lba >> 8) & 0xFF));
    outb(base + 5, (uint8_t)((lba >> 16) & 0xFF));
    outb(base + 6, 0xE0 | (drive_sel << 4) | ((lba >> 24) & 0x0F));
    outb(base + 7, write ? IDE_CMD_WRITE_SECTORS : IDE_CMD_READ_SECTORS);
    
    ide_wait_400ns(base);
    
    uint16_t *buf = (uint16_t*)buffer;
    for (uint16_t i = 0; i < count; i++) {
        if (ide_wait_ready_impl(base, 1000000) != 0) {
            return -1;
        }
        
        if (write) {
            for (int j = 0; j < 256; j++) {
                outw(base, buf[i * 256 + j]);
            }
            ide_wait_400ns(base);
        } else {
            for (int j = 0; j < 256; j++) {
                buf[i * 256 + j] = inw(base);
            }
            ide_wait_400ns(base);
        }
    }
    
    if (write) {
        outb(base + 7, 0xE7);
        ide_wait_ready_impl(base, 1000000);
    }
    
    return 0;
}

int ide_read_sectors(uint8_t drive, uint32_t lba, uint16_t count, void *buffer) {
    return ide_read_write_sectors(drive, lba, count, buffer, 0);
}

int ide_write_sectors(uint8_t drive, uint32_t lba, uint16_t count, const void *buffer) {
    return ide_read_write_sectors(drive, lba, count, (void*)buffer, 1);
}

void ide_irq_handler(void) {
    ide_irq_fired = 1;
    ide_last_status = inb(IDE_PRIMARY_BASE + 7);
    if (ide_last_status & IDE_STATUS_ERR) {
        ide_last_error = inb(IDE_PRIMARY_BASE + 1);
    }
    outb(PIC1_COMMAND, PIC_EOI);
    outb(PIC2_COMMAND, PIC_EOI);
}

int ide_wait_ready(uint16_t base, uint32_t timeout) {
    return ide_wait_ready_impl(base, timeout);
}

uint16_t inw(uint16_t port) {
    uint16_t value;
    __asm__ volatile("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void outw(uint16_t port, uint16_t value) {
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}
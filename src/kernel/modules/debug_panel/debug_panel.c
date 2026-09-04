#include "debug_panel.h"
#include "../framebuffer/framebuffer.h"
#include "../sysinfo/sysinfo.h"
#include "../pci/pci.h"
#include "../ide/ide.h"
#include "../mem/mem.h"
#include "../mem/pmm.h"
#include "../timer/timer.h"
#include "../interrupts/interrupts.h"
#include "../../string.h"
#include <stdint.h>

#define COM1 0x3F8

static void dbg_putc(char c) {
    while ((inb(COM1 + 5) & 0x20) == 0);
    outb(COM1, c);
}
void dbg(const char *s) { while (*s) { if (*s=='\n') dbg_putc('\r'); dbg_putc(*s++); } }

#define PANEL_X 16
#define PANEL_MARGIN_RIGHT 16

static int test_pmm_status;
static int test_vmm_status;

static void str_append(char *buf, uint64_t *pos, uint64_t cap, const char *s) {
    while (*s && *pos + 1 < cap)
        buf[(*pos)++] = *s++;
}

static void str_append_dec(char *buf, uint64_t *pos, uint64_t cap, uint64_t value) {
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

static void str_append_hex(char *buf, uint64_t *pos, uint64_t cap, uint64_t value) {
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

static uint64_t draw_line(const char *label, const char *value, uint64_t y, uint32_t label_color, uint32_t value_color) {
    (void)value_color;
    char line[256];
    uint64_t pos = 0;
    str_append(line, &pos, sizeof(line), "  ");
    str_append(line, &pos, sizeof(line), label);
    str_append(line, &pos, sizeof(line), ": ");
    str_append(line, &pos, sizeof(line), value);
    line[pos < sizeof(line) ? pos : sizeof(line) - 1] = '\0';
    fb_draw_string(line, PANEL_X, y, label_color, FB_COLOR_BLACK);
    return y + 16;
}

static uint64_t draw_section_header(const char *title, uint64_t y) {
    fb_draw_string(title, PANEL_X, y, FB_COLOR_CYAN, FB_COLOR_BLACK);
    return y + 16;
}

void debug_panel_init(int pmm_status, int vmm_status) {
    test_pmm_status = pmm_status;
    test_vmm_status = vmm_status;
}

static uint64_t format_uptime(uint64_t seconds, char *buf) {
    uint64_t h = seconds / 3600;
    uint64_t m = (seconds % 3600) / 60;
    uint64_t s = seconds % 60;
    uint64_t pos = 0;

    if (h < 10) buf[pos++] = '0';
    str_append_dec(buf, &pos, 64, h);
    buf[pos++] = ':';
    if (m < 10) buf[pos++] = '0';
    str_append_dec(buf, &pos, 64, m);
    buf[pos++] = ':';
    if (s < 10) buf[pos++] = '0';
    str_append_dec(buf, &pos, 64, s);
    buf[pos] = '\0';
    return pos;
}

static uint64_t format_size_mb(uint64_t bytes, char *buf) {
    uint64_t pos = 0;
    str_append_dec(buf, &pos, 64, bytes / (1024 * 1024));
    str_append(buf, &pos, 64, " MB");
    buf[pos] = '\0';
    return pos;
}

void debug_panel_render(void) {
    dbg("[DP] render start\n");
    fb_clear(FB_COLOR_BLACK);
    fb_draw_color_bar(0, 40);
    dbg("[DP] colorbar\n");

    uint64_t y = 56;
    uint64_t x = PANEL_X;
    char buf[256];
    uint64_t pos;

    fb_draw_string("OS44 DEBUG PANEL", x, y, FB_COLOR_WHITE, FB_COLOR_BLACK);
    y += 20;

    uint64_t uptime;
    sysinfo_get_uptime(&uptime);
    pos = 0;
    str_append(buf, &pos, sizeof(buf), "Uptime: ");
    format_uptime(uptime, buf + pos);
    while (buf[pos]) pos++;
    fb_draw_string(buf, x, y, FB_COLOR_WHITE, FB_COLOR_BLACK);
    y += 20;

    y = draw_section_header("--- CPU ---", y);

    char vendor[16];
    sysinfo_get_cpu_vendor(vendor);
    y = draw_line("Vendor", vendor, y, FB_COLOR_WHITE, FB_COLOR_GREEN);

    char brand[49];
    sysinfo_get_cpu_brand(brand);
    dbg("[DP] cpu_brand: "); dbg(brand); dbg("\n");
    y = draw_line("Brand", brand, y, FB_COLOR_WHITE, FB_COLOR_GREEN);

    char cores_buf[32];
    pos = 0;
    str_append_dec(cores_buf, &pos, sizeof(cores_buf), sysinfo_get_core_count());
    cores_buf[pos] = '\0';
    y = draw_line("Logical Cores", cores_buf, y, FB_COLOR_WHITE, FB_COLOR_GREEN);

    char features[256];
    sysinfo_get_cpu_features(features, sizeof(features));
    dbg("[DP] features: "); dbg(features); dbg("\n");
    if (features[0]) {
        y = draw_line("Features", features, y, FB_COLOR_WHITE, FB_COLOR_YELLOW);
    }
    y += 4;

    y = draw_section_header("--- MEMORY ---", y);

    uint64_t mem_used, mem_total;
    sysinfo_get_mem_usage(&mem_used, &mem_total);
    char size_buf[32];
    format_size_mb(mem_total, size_buf);
    y = draw_line("Total RAM", size_buf, y, FB_COLOR_WHITE, FB_COLOR_GREEN);
    format_size_mb(mem_used, size_buf);
    y = draw_line("Used", size_buf, y, FB_COLOR_WHITE, FB_COLOR_YELLOW);
    uint64_t mem_free = mem_total - mem_used;
    format_size_mb(mem_free, size_buf);
    y = draw_line("Free", size_buf, y, FB_COLOR_WHITE, FB_COLOR_GREEN);

    char frames_buf[32];
    pos = 0;
    str_append_dec(frames_buf, &pos, sizeof(frames_buf), pmm_get_free_frames());
    str_append(frames_buf, &pos, sizeof(frames_buf), " free pages");
    frames_buf[pos] = '\0';
    dbg("[DP] pmm frames\n");
    y = draw_line("PMM", frames_buf, y, FB_COLOR_WHITE, FB_COLOR_WHITE);
    y += 4;

    y = draw_section_header("--- DISPLAY ---", y);

    uint64_t w, h;
    sysinfo_get_screen_res(&w, &h);
    char res_buf[64];
    pos = 0;
    str_append_dec(res_buf, &pos, sizeof(res_buf), w);
    res_buf[pos++] = 'x';
    str_append_dec(res_buf, &pos, sizeof(res_buf), h);
    res_buf[pos] = '\0';
    y = draw_line("Resolution", res_buf, y, FB_COLOR_WHITE, FB_COLOR_GREEN);

    char pitch_buf[32];
    pos = 0;
    str_append_dec(pitch_buf, &pos, sizeof(pitch_buf), fb_pitch());
    str_append(pitch_buf, &pos, sizeof(pitch_buf), " bytes");
    pitch_buf[pos] = '\0';
    y = draw_line("Pitch", pitch_buf, y, FB_COLOR_WHITE, FB_COLOR_WHITE);

    y = draw_line("Format", "BGR 8-bit", y, FB_COLOR_WHITE, FB_COLOR_WHITE);
    y += 4;

    y = draw_section_header("--- FIRMWARE ---", y);

    char fw_vendor[33];
    sysinfo_get_uefi_vendor(fw_vendor);
    y = draw_line("UEFI Vendor", fw_vendor, y, FB_COLOR_WHITE, FB_COLOR_WHITE);

    char rev_buf[32];
    pos = 0;
    str_append_hex(rev_buf, &pos, sizeof(rev_buf), sysinfo_get_uefi_revision());
    rev_buf[pos] = '\0';
    y = draw_line("Revision", rev_buf, y, FB_COLOR_WHITE, FB_COLOR_WHITE);

    y = draw_line("MMap Size", "", y, FB_COLOR_WHITE, FB_COLOR_WHITE);
    y += 4;

    y = draw_section_header("--- STORAGE DEVICES ---", y);

    if (ide_primary_master) {
        pos = 0;
        str_append(buf, &pos, sizeof(buf), "[IDE] Primary Master   : ");
        str_append_dec(buf, &pos, sizeof(buf), ide_primary_master_sectors / 2048);
        str_append(buf, &pos, sizeof(buf), " MB");
        buf[pos] = '\0';
        y = draw_line("", buf + 2, y, FB_COLOR_GREEN, FB_COLOR_WHITE);
    } else {
        y = draw_line("", "[IDE] Primary Master   : Not present", y, FB_COLOR_RED, FB_COLOR_RED);
    }
    if (ide_primary_slave) {
        pos = 0;
        str_append(buf, &pos, sizeof(buf), "[IDE] Primary Slave    : ");
        str_append_dec(buf, &pos, sizeof(buf), ide_primary_slave_sectors / 2048);
        str_append(buf, &pos, sizeof(buf), " MB");
        buf[pos] = '\0';
        y = draw_line("", buf + 2, y, FB_COLOR_GREEN, FB_COLOR_WHITE);
    } else {
        y = draw_line("", "[IDE] Primary Slave    : Not present", y, FB_COLOR_RED, FB_COLOR_RED);
    }
    if (ide_secondary_master) {
        pos = 0;
        str_append(buf, &pos, sizeof(buf), "[IDE] Secondary Master : ");
        str_append_dec(buf, &pos, sizeof(buf), ide_secondary_master_sectors / 2048);
        str_append(buf, &pos, sizeof(buf), " MB");
        buf[pos] = '\0';
        y = draw_line("", buf + 2, y, FB_COLOR_GREEN, FB_COLOR_WHITE);
    } else {
        y = draw_line("", "[IDE] Secondary Master : Not present", y, FB_COLOR_RED, FB_COLOR_RED);
    }
    if (ide_secondary_slave) {
        pos = 0;
        str_append(buf, &pos, sizeof(buf), "[IDE] Secondary Slave  : ");
        str_append_dec(buf, &pos, sizeof(buf), ide_secondary_slave_sectors / 2048);
        str_append(buf, &pos, sizeof(buf), " MB");
        buf[pos] = '\0';
        y = draw_line("", buf + 2, y, FB_COLOR_GREEN, FB_COLOR_WHITE);
    } else {
        y = draw_line("", "[IDE] Secondary Slave  : Not present", y, FB_COLOR_RED, FB_COLOR_RED);
    }

    int found_nvme = 0;
    for (int i = 0; i < pci_device_count; i++) {
        pci_device_t *dev = &pci_devices[i];
        if (dev->class == PCI_CLASS_MASS_STORAGE && dev->subclass == PCI_SUBCLASS_NVME) {            found_nvme = 1;
            pos = 0;
            str_append(buf, &pos, sizeof(buf), "[NVMe] ");
            str_append_hex(buf, &pos, sizeof(buf), dev->vendor_id);
            str_append(buf, &pos, sizeof(buf), ":");
            str_append_hex(buf, &pos, sizeof(buf), dev->device_id);
            buf[pos] = '\0';
            y = draw_line("", buf + 2, y, FB_COLOR_GREEN, FB_COLOR_WHITE);
        }
    }
    if (!found_nvme) {
        y = draw_line("", "[NVMe] No NVMe devices found", y, FB_COLOR_RED, FB_COLOR_RED);
    }

    int found_sata = 0;
    for (int i = 0; i < pci_device_count; i++) {
        pci_device_t *dev = &pci_devices[i];
        if (dev->class == PCI_CLASS_MASS_STORAGE && dev->subclass == PCI_SUBCLASS_SATA_AHCI) {
            found_sata = 1;
            pos = 0;
            str_append(buf, &pos, sizeof(buf), "[SATA] AHCI Controller at ");
            str_append_hex(buf, &pos, sizeof(buf), dev->bar0 & ~0xF);
            buf[pos] = '\0';
            y = draw_line("", buf + 2, y, FB_COLOR_GREEN, FB_COLOR_WHITE);
        }
    }
    if (!found_sata) {
        y = draw_line("", "[SATA] No AHCI controllers found", y, FB_COLOR_RED, FB_COLOR_RED);
    }
    y += 4;

    y = draw_section_header("--- PCI DEVICES ---", y);

    if (pci_device_count == 0) {
        y = draw_line("No PCI devices detected", "", y, FB_COLOR_RED, FB_COLOR_RED);
    } else {
        for (int i = 0; i < pci_device_count && y < fb_height() - 16; i++) {
            pci_device_t *dev = &pci_devices[i];
            pos = 0;
            char addr[8];
            addr[0] = (dev->bus >> 4) < 10 ? '0' + (dev->bus >> 4) : 'A' + (dev->bus >> 4) - 10;
            addr[1] = (dev->bus & 0xF) < 10 ? '0' + (dev->bus & 0xF) : 'A' + (dev->bus & 0xF) - 10;
            addr[2] = ':';
            addr[3] = ((dev->device >> 4) < 10) ? '0' + (dev->device >> 4) : 'A' + (dev->device >> 4) - 10;
            addr[4] = (dev->device & 0xF) < 10 ? '0' + (dev->device & 0xF) : 'A' + (dev->device & 0xF) - 10;
            addr[5] = '.';
            addr[6] = '0' + dev->function;
            addr[7] = '\0';

            str_append(buf, &pos, sizeof(buf), addr);
            str_append(buf, &pos, sizeof(buf), " ");
            str_append(buf, &pos, sizeof(buf), pci_class_name(dev->class, dev->subclass, dev->prog_if));
            str_append(buf, &pos, sizeof(buf), " [");
            str_append_hex(buf, &pos, sizeof(buf), dev->vendor_id);
            str_append(buf, &pos, sizeof(buf), ":");
            str_append_hex(buf, &pos, sizeof(buf), dev->device_id);
            str_append(buf, &pos, sizeof(buf), "]");

            if (dev->irq_line != 0 && dev->irq_line != 0xFF) {
                str_append(buf, &pos, sizeof(buf), " IRQ:");
                str_append_dec(buf, &pos, sizeof(buf), dev->irq_line);
            }

            buf[pos < sizeof(buf) ? pos : sizeof(buf) - 1] = '\0';

            uint32_t color = FB_COLOR_WHITE;
            if (dev->class == PCI_CLASS_DISPLAY) color = FB_COLOR_GREEN;
            else if (dev->class == PCI_CLASS_NETWORK) color = FB_COLOR_CYAN;
            else if (dev->class == PCI_CLASS_MASS_STORAGE) color = FB_COLOR_YELLOW;

            fb_draw_string("  ", x, y, FB_COLOR_BLACK, FB_COLOR_BLACK);
            fb_draw_string(buf, x + 16, y, color, FB_COLOR_BLACK);
            y += 14;
        }
        dbg("[DP] pci devices drawn\n");
    }
    y += 4;

    y = draw_section_header("--- SELF-TESTS ---", y);

    if (test_pmm_status == 1)
        y = draw_line("PMM Test", "PASS", y, FB_COLOR_WHITE, FB_COLOR_GREEN);
    else if (test_pmm_status == 2)
        y = draw_line("PMM Test", "FAIL", y, FB_COLOR_WHITE, FB_COLOR_RED);
    else
        y = draw_line("PMM Test", "TESTING...", y, FB_COLOR_WHITE, FB_COLOR_WHITE);

    if (test_vmm_status == 1)
        y = draw_line("VMM Test", "PASS", y, FB_COLOR_WHITE, FB_COLOR_GREEN);
    else if (test_vmm_status == 2)
        y = draw_line("VMM Test", "FAIL", y, FB_COLOR_WHITE, FB_COLOR_RED);
    else
        y = draw_line("VMM Test", "TESTING...", y, FB_COLOR_WHITE, FB_COLOR_WHITE);

    y += 4;
    y = draw_section_header("--- FRAME COUNTER ---", y);
    static uint64_t frame_counter = 0;
    frame_counter++;
    pos = 0;
    str_append(buf, &pos, sizeof(buf), "Frame: ");
    str_append_dec(buf, &pos, sizeof(buf), frame_counter);
    buf[pos] = '\0';
    fb_draw_string(buf, x, y, FB_COLOR_WHITE, FB_COLOR_BLACK);
    dbg("[DP] render end\n");
}

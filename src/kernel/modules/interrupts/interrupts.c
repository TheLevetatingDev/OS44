#include "interrupts.h"
#include "../framebuffer/framebuffer.h"
#include "../keyboard/keyboard.h"
#include "../timer/timer.h"
#include <stdint.h>

#define IDT_ENTRIES 256

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

#define PIC_EOI      0x20

#define ICW1_INIT    0x11
#define ICW4_8086    0x01

#define ISR_CPU_MAX  32
#define ISR_IRQ_BASE 32
#define ISR_IRQ_MAX  48

typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed)) IdtEntry;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) IdtPointer;

typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rsi, rdi, rbp, rdx, rcx, rbx, rax; /* must match isr_common_stub push order */
    uint64_t vector, error_code;
    uint64_t rip, cs, rflags, rsp, ss;
} IntrFrame;

static IdtEntry idt[IDT_ENTRIES];

extern void isr_0(void);
extern void isr_1(void);
extern void isr_2(void);
extern void isr_3(void);
extern void isr_4(void);
extern void isr_5(void);
extern void isr_6(void);
extern void isr_7(void);
extern void isr_8(void);
extern void isr_9(void);
extern void isr_10(void);
extern void isr_11(void);
extern void isr_12(void);
extern void isr_13(void);
extern void isr_14(void);
extern void isr_15(void);
extern void isr_16(void);
extern void isr_17(void);
extern void isr_18(void);
extern void isr_19(void);
extern void isr_20(void);
extern void isr_21(void);
extern void isr_22(void);
extern void isr_23(void);
extern void isr_24(void);
extern void isr_25(void);
extern void isr_26(void);
extern void isr_27(void);
extern void isr_28(void);
extern void isr_29(void);
extern void isr_30(void);
extern void isr_31(void);
extern void isr_32(void);
extern void isr_33(void);
extern void isr_34(void);
extern void isr_35(void);
extern void isr_36(void);
extern void isr_37(void);
extern void isr_38(void);
extern void isr_39(void);
extern void isr_40(void);
extern void isr_41(void);
extern void isr_42(void);
extern void isr_43(void);
extern void isr_44(void);
extern void isr_45(void);
extern void isr_46(void);
extern void isr_47(void);
extern void isr_unhandled(void);

static const char *const exception_names[ISR_CPU_MAX] = {
    "Divide Error (#DE)",
    "Debug (#DB)",
    "Non-Maskable Interrupt",
    "Breakpoint (#BP)",
    "Overflow (#OF)",
    "Bound Range (#BR)",
    "Invalid Opcode (#UD)",
    "Device Not Available (#NM)",
    "Double Fault (#DF)",
    "Coprocessor Segment Overrun",
    "Invalid TSS (#TS)",
    "Segment Not Present (#NP)",
    "Stack Fault (#SS)",
    "General Protection (#GP)",
    "Page Fault (#PF)",
    "Reserved",
    "x87 FPU Error (#MF)",
    "Alignment Check (#AC)",
    "Machine Check (#MC)",
    "SIMD FPU Error (#XM)",
    "Virtualization (#VE)",
    "Control Protection (#CP)",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved",
};

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

static void append_str(char *buf, uint64_t *pos, uint64_t cap, const char *s) {
    while (*s && *pos + 1 < cap)
        buf[(*pos)++] = *s++;
}

void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static uint16_t read_cs(void) {
    uint16_t cs;
    __asm__ volatile("movw %%cs, %0" : "=r"(cs));
    return cs;
}

static void hex_to_string_16(char *buf, uint16_t value) {
    const char hex_chars[] = "0123456789ABCDEF";
    buf[0] = hex_chars[(value >> 12) & 0xF];
    buf[1] = hex_chars[(value >> 8) & 0xF];
    buf[2] = hex_chars[(value >> 4) & 0xF];
    buf[3] = hex_chars[value & 0xF];
    buf[4] = '\0';
}

static void idt_set_gate(unsigned int vector, void (*handler)(void)) {
    uint64_t addr = (uint64_t)handler;
    idt[vector].offset_low  = (uint16_t)(addr & 0xFFFF);
    idt[vector].offset_mid  = (uint16_t)((addr >> 16) & 0xFFFF);
    idt[vector].offset_high = (uint32_t)(addr >> 32);
    uint16_t cs = read_cs();
    idt[vector].selector    = cs;
    idt[vector].ist         = 0;
    idt[vector].type_attr   = 0x8E;
    idt[vector].zero        = 0;

    if (vector == ISR_IRQ_BASE + 1) { // Debug for keyboard interrupt
        char cs_str[5];
        hex_to_string_16(cs_str, cs);
        fb_draw_string("IDT entry 33: CS=", 16, 432, FB_COLOR_WHITE, FB_COLOR_BLACK);
        fb_draw_string(cs_str, 16 + (8 * 17), 432, FB_COLOR_WHITE, FB_COLOR_BLACK);

        char addr_str[17];
        // This is a simple conversion, a full uint64_t to hex would be more involved
        // For debugging, we can just print the lower 32 bits if needed due to fb_draw_string limits.
        // For now, let's just print the CS as that's a more common issue.
        // Re-using append_hex from interrupts.c for addr
        uint64_t pos = 0;
        append_hex(addr_str, &pos, sizeof(addr_str), addr);
        addr_str[pos < sizeof(addr_str) ? pos : sizeof(addr_str) - 1] = '\0';
        fb_draw_string(" Handler addr=", 16 + (8 * 23), 432, FB_COLOR_WHITE, FB_COLOR_BLACK);
        fb_draw_string(addr_str, 16 + (8 * 37), 432, FB_COLOR_WHITE, FB_COLOR_BLACK);
    }
}

static void idt_load(void) {
    IdtPointer ptr;
    ptr.limit = (uint16_t)(sizeof(idt) - 1);
    ptr.base  = (uint64_t)idt;
    __asm__ volatile("lidt %0" : : "m"(ptr));
}

static void pic_remap(uint8_t offset1, uint8_t offset2) {
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);

    outb(PIC1_COMMAND, ICW1_INIT);
    outb(PIC2_COMMAND, ICW1_INIT);
    outb(PIC1_DATA, offset1);
    outb(PIC2_DATA, offset2);
    outb(PIC1_DATA, 4);
    outb(PIC2_DATA, 2);
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

static void pic_mask_all(void) {
    outb(PIC1_DATA, 0xFE);
    outb(PIC2_DATA, 0xFF);
}

static void pic_eoi(uint64_t vector) {
    if (vector >= ISR_IRQ_BASE && vector < ISR_IRQ_MAX) {
        if (vector >= 40)
            outb(PIC2_COMMAND, PIC_EOI);
        outb(PIC1_COMMAND, PIC_EOI);
    }
}

static void panic_report(const IntrFrame *frame) {
    __asm__ volatile("cli");

    fb_clear(FB_COLOR_BLACK);
    fb_draw_string("*** CPU EXCEPTION ***", 16, 16, FB_COLOR_WHITE, FB_COLOR_BLACK);

    const char *name = "Unknown exception";
    if (frame->vector < ISR_CPU_MAX)
        name = exception_names[frame->vector];

    fb_draw_string(name, 16, 32, 0x00FF5555, FB_COLOR_BLACK);

    char line[72];
    uint64_t pos;

    pos = 0;
    append_str(line, &pos, sizeof(line), "Vector ");
    append_dec(line, &pos, sizeof(line), frame->vector);
    line[pos < sizeof(line) ? pos : sizeof(line) - 1] = '\0';
    fb_draw_string(line, 16, 56, FB_COLOR_WHITE, FB_COLOR_BLACK);

    pos = 0;
    append_str(line, &pos, sizeof(line), "Error code ");
    append_hex(line, &pos, sizeof(line), frame->error_code);
    line[pos < sizeof(line) ? pos : sizeof(line) - 1] = '\0';
    fb_draw_string(line, 16, 72, FB_COLOR_WHITE, FB_COLOR_BLACK);

    pos = 0;
    append_str(line, &pos, sizeof(line), "RIP ");
    append_hex(line, &pos, sizeof(line), frame->rip);
    line[pos < sizeof(line) ? pos : sizeof(line) - 1] = '\0';
    fb_draw_string(line, 16, 96, FB_COLOR_WHITE, FB_COLOR_BLACK);

    pos = 0;
    append_str(line, &pos, sizeof(line), "CS ");
    append_hex(line, &pos, sizeof(line), frame->cs);
    append_str(line, &pos, sizeof(line), "  RFLAGS ");
    append_hex(line, &pos, sizeof(line), frame->rflags);
    line[pos < sizeof(line) ? pos : sizeof(line) - 1] = '\0';
    fb_draw_string(line, 16, 112, FB_COLOR_WHITE, FB_COLOR_BLACK);

    pos = 0;
    append_str(line, &pos, sizeof(line), "RSP ");
    append_hex(line, &pos, sizeof(line), frame->rsp);
    append_str(line, &pos, sizeof(line), "  SS ");
    append_hex(line, &pos, sizeof(line), frame->ss);
    line[pos < sizeof(line) ? pos : sizeof(line) - 1] = '\0';
    fb_draw_string(line, 16, 128, FB_COLOR_WHITE, FB_COLOR_BLACK);

    if (frame->vector == 14) {
        uint64_t cr2;
        __asm__ volatile("movq %%cr2, %0" : "=r"(cr2));
        pos = 0;
        append_str(line, &pos, sizeof(line), "CR2 (fault addr) ");
        append_hex(line, &pos, sizeof(line), cr2);
        line[pos < sizeof(line) ? pos : sizeof(line) - 1] = '\0';
        fb_draw_string(line, 16, 152, 0x00FFFF00, FB_COLOR_BLACK);
    }

    pos = 0;
    append_str(line, &pos, sizeof(line), "RAX ");
    append_hex(line, &pos, sizeof(line), frame->rax);
    append_str(line, &pos, sizeof(line), " RBX ");
    append_hex(line, &pos, sizeof(line), frame->rbx);
    line[pos < sizeof(line) ? pos : sizeof(line) - 1] = '\0';
    fb_draw_string(line, 16, 184, FB_COLOR_WHITE, FB_COLOR_BLACK);

    pos = 0;
    append_str(line, &pos, sizeof(line), "RCX ");
    append_hex(line, &pos, sizeof(line), frame->rcx);
    append_str(line, &pos, sizeof(line), " RDX ");
    append_hex(line, &pos, sizeof(line), frame->rdx);
    line[pos < sizeof(line) ? pos : sizeof(line) - 1] = '\0';
    fb_draw_string(line, 16, 200, FB_COLOR_WHITE, FB_COLOR_BLACK);

    fb_draw_string("System halted.", 16, 232, FB_COLOR_GREEN, FB_COLOR_BLACK);

    while (1)
        __asm__ volatile("hlt");
}

void isr_handler(IntrFrame *frame) {
    if (frame->vector < ISR_CPU_MAX) {
        panic_report(frame);
        return; /* unreachable, but prevents fall-through if ever refactored */
    }

    if (frame->vector >= ISR_IRQ_BASE && frame->vector < ISR_IRQ_MAX) {
        pic_eoi(frame->vector);
        if (frame->vector == ISR_IRQ_BASE + 0) { /* Timer IRQ0 */
            timer_handler();
        } else if (frame->vector == ISR_IRQ_BASE + 1) { /* Keyboard IRQ1 */
            fb_draw_string("Keyboard IRQ received!", 16, 250, FB_COLOR_WHITE, FB_COLOR_BLACK);
            keyboard_handler();
        }
        return;
    }

    if (frame->vector == 0xFFFF)
        return;

    panic_report(frame);
}

void intr_init(void) {
    static void (*const cpu_stubs[ISR_CPU_MAX])(void) = {
        isr_0,  isr_1,  isr_2,  isr_3,  isr_4,  isr_5,  isr_6,  isr_7,
        isr_8,  isr_9,  isr_10, isr_11, isr_12, isr_13, isr_14, isr_15,
        isr_16, isr_17, isr_18, isr_19, isr_20, isr_21, isr_22, isr_23,
        isr_24, isr_25, isr_26, isr_27, isr_28, isr_29, isr_30, isr_31,
    };

    static void (*const irq_stubs[16])(void) = {
        isr_32, isr_33, isr_34, isr_35, isr_36, isr_37, isr_38, isr_39,
        isr_40, isr_41, isr_42, isr_43, isr_44, isr_45, isr_46, isr_47,
    };

    for (unsigned int i = 0; i < IDT_ENTRIES; i++)
        idt_set_gate(i, isr_unhandled);

    for (unsigned int i = 0; i < ISR_CPU_MAX; i++)
        idt_set_gate(i, cpu_stubs[i]);

    for (unsigned int i = 0; i < 16; i++)
        idt_set_gate(ISR_IRQ_BASE + i, irq_stubs[i]);

    pic_remap(ISR_IRQ_BASE, ISR_IRQ_BASE + 8);
    /* Mask all IRQs — keyboard_init() will unmask IRQ1, then kernel_main calls sti */
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);

    idt_load();
    /* NOTE: caller must execute sti after all devices are initialised */
}

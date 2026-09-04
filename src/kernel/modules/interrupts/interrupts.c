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

#define KERNEL_CS 0x08
#define KERNEL_DS 0x10

static uint64_t gdt[3];

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) Gdtr;

static void gdt_load(void) {
    Gdtr gdtr;
    gdtr.limit = (uint16_t)(sizeof(gdt) - 1);
    gdtr.base  = (uint64_t)gdt;
    __asm__ volatile("lgdt %0" : : "m"(gdtr));

    __asm__ volatile(
        "pushq %0\n"
        "leaq 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:\n"
        "movw %w1, %%ss\n"
        "movw %w1, %%ds\n"
        "movw %w1, %%es\n"
        "movw %w1, %%fs\n"
        "movw %w1, %%gs\n"
        :
        : "r"((uint64_t)KERNEL_CS), "r"((uint16_t)KERNEL_DS)
        : "rax");
}

static void gdt_init(void) {
    gdt[0] = 0x0000000000000000ULL;  /* null */
    gdt[1] = 0x00AF9A000000FFFFULL;  /* kernel code, 64-bit, ring 0 */
    gdt[2] = 0x00CF92000000FFFFULL;  /* kernel data, ring 0 */
    gdt_load();
}

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

void outw(uint16_t port, uint16_t value) {
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

uint16_t inw(uint16_t port) {
    uint16_t value;
    __asm__ volatile("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void outl(uint16_t port, uint32_t value) {
    __asm__ volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

uint32_t inl(uint16_t port) {
    uint32_t value;
    __asm__ volatile("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static uint16_t read_cs(void) {
    uint16_t cs;
    __asm__ volatile("movw %%cs, %0" : "=r"(cs));
    return cs;
}

static void idt_set_gate(unsigned int vector, void (*handler)(void)) {
    uint64_t addr = (uint64_t)handler;
    idt[vector].offset_low  = (uint16_t)(addr & 0xFFFF);
    idt[vector].offset_mid  = (uint16_t)((addr >> 16) & 0xFFFF);
    idt[vector].offset_high = (uint32_t)(addr >> 32);
    uint16_t cs = KERNEL_CS;
    idt[vector].selector    = cs;
    idt[vector].ist         = 0;
    idt[vector].type_attr   = 0x8E;
    idt[vector].zero        = 0;
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
    {
        uint64_t v = frame->vector;
        outb(0x3F8, 'I'); outb(0x3F8, '=');
        char b[21]; char t[20]; int bi=0, tj=0;
        if (v==0) b[bi++]='0';
        while(v>0){t[tj++]='0'+v%10;v/=10;}
        while(tj>0)b[bi++]=t[--tj];
        b[bi]='\0';
        for(int i=0;i<bi;i++) outb(0x3F8,b[i]);
        outb(0x3F8, '\n');
    }
    if (frame->vector < ISR_CPU_MAX) {
        panic_report(frame);
        return; /* unreachable, but prevents fall-through if ever refactored */
    }

    if (frame->vector >= ISR_IRQ_BASE && frame->vector < ISR_IRQ_MAX) {
        pic_eoi(frame->vector);
        if (frame->vector == ISR_IRQ_BASE + 0) { /* Timer IRQ0 */
            timer_handler();
        } else if (frame->vector == ISR_IRQ_BASE + 1) { /* Keyboard IRQ1 */
            keyboard_handler();
        }
        return;
    }

    if (frame->vector == 0xFFFF)
        return;

    panic_report(frame);
}

void intr_init(void) {
    gdt_init();

    for (unsigned int i = 0; i < IDT_ENTRIES; i++)
        idt_set_gate(i, isr_unhandled);

    idt_set_gate(0, isr_0);
    idt_set_gate(1, isr_1);
    idt_set_gate(2, isr_2);
    idt_set_gate(3, isr_3);
    idt_set_gate(4, isr_4);
    idt_set_gate(5, isr_5);
    idt_set_gate(6, isr_6);
    idt_set_gate(7, isr_7);
    idt_set_gate(8, isr_8);
    idt_set_gate(9, isr_9);
    idt_set_gate(10, isr_10);
    idt_set_gate(11, isr_11);
    idt_set_gate(12, isr_12);
    idt_set_gate(13, isr_13);
    idt_set_gate(14, isr_14);
    idt_set_gate(15, isr_15);
    idt_set_gate(16, isr_16);
    idt_set_gate(17, isr_17);
    idt_set_gate(18, isr_18);
    idt_set_gate(19, isr_19);
    idt_set_gate(20, isr_20);
    idt_set_gate(21, isr_21);
    idt_set_gate(22, isr_22);
    idt_set_gate(23, isr_23);
    idt_set_gate(24, isr_24);
    idt_set_gate(25, isr_25);
    idt_set_gate(26, isr_26);
    idt_set_gate(27, isr_27);
    idt_set_gate(28, isr_28);
    idt_set_gate(29, isr_29);
    idt_set_gate(30, isr_30);
    idt_set_gate(31, isr_31);

    idt_set_gate(32, isr_32);
    idt_set_gate(33, isr_33);
    idt_set_gate(34, isr_34);
    idt_set_gate(35, isr_35);
    idt_set_gate(36, isr_36);
    idt_set_gate(37, isr_37);
    idt_set_gate(38, isr_38);
    idt_set_gate(39, isr_39);
    idt_set_gate(40, isr_40);
    idt_set_gate(41, isr_41);
    idt_set_gate(42, isr_42);
    idt_set_gate(43, isr_43);
    idt_set_gate(44, isr_44);
    idt_set_gate(45, isr_45);
    idt_set_gate(46, isr_46);
    idt_set_gate(47, isr_47);

    pic_remap(ISR_IRQ_BASE, ISR_IRQ_BASE + 8);
    /* Mask all IRQs — keyboard_init() will unmask IRQ1, then kernel_main calls sti */
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);

    idt_load();
    /* NOTE: caller must execute sti after all devices are initialised */
}

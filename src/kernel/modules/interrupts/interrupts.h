#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1
#define PIC_EOI      0x20
#define ICW1_INIT    0x11
#define ICW4_8086    0x01
#define ISR_IRQ_BASE 32
#define ISR_IRQ_MAX  48

void intr_init(void);
void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);
void outl(uint16_t port, uint32_t value);
uint32_t inl(uint16_t port);
void outw(uint16_t port, uint16_t value);
uint16_t inw(uint16_t port);

#endif

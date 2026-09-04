#ifndef IDE_H
#define IDE_H

#include <stdint.h>
#include <stddef.h>

#define IDE_PRIMARY_BASE    0x1F0
#define IDE_PRIMARY_CTRL    0x3F6
#define IDE_SECONDARY_BASE  0x170
#define IDE_SECONDARY_CTRL  0x376

#define IDE_CMD_READ_SECTORS     0x20
#define IDE_CMD_WRITE_SECTORS    0x30
#define IDE_CMD_IDENTIFY         0xEC
#define IDE_CMD_READ_DMA         0xC8
#define IDE_CMD_WRITE_DMA        0xCA

#define IDE_STATUS_BSY  0x80
#define IDE_STATUS_DRDY 0x40
#define IDE_STATUS_DRQ  0x08
#define IDE_STATUS_ERR  0x01

#define IDE_PRIMARY_IRQ  14
#define IDE_SECONDARY_IRQ 15

#define IDE_SECTOR_SIZE 512

typedef struct {
    uint16_t config;
    uint16_t cylinders;
    uint16_t reserved1;
    uint16_t heads;
    uint16_t sectors_per_track;
    uint16_t vendor_unique[3];
    uint8_t  serial_number[20];
    uint16_t buffer_type;
    uint16_t buffer_size;
    uint16_t ecc_bytes;
    uint8_t  firmware_revision[8];
    uint8_t  model_number[40];
    uint16_t max_multiple_sectors;
    uint16_t vendor_unique2;
    uint16_t capabilities;
    uint16_t reserved2;
    uint16_t pio_timing;
    uint16_t dma_timing;
    uint16_t field_valid;
    uint16_t current_cylinders;
    uint16_t current_heads;
    uint16_t current_sectors;
    uint32_t current_capacity;
    uint8_t  multiple_sectors;
    uint32_t total_lba28;
    uint16_t multi_dma;
    uint16_t mfr_pio;
    uint16_t mfr_dma;
    uint16_t reserved3[196];
} __attribute__((packed)) ide_identify_t;

void ide_init(void);
int ide_detect_drives(void);
int ide_read_sectors(uint8_t drive, uint32_t lba, uint16_t count, void *buffer);
int ide_write_sectors(uint8_t drive, uint32_t lba, uint16_t count, const void *buffer);
void ide_identify(uint8_t drive, ide_identify_t *identify);
int ide_wait_ready(uint16_t base, uint32_t timeout);
void ide_irq_handler(void);

extern volatile int ide_irq_fired;
extern volatile int ide_last_status;
extern volatile int ide_last_error;

extern uint8_t ide_primary_master;
extern uint8_t ide_primary_slave;
extern uint8_t ide_secondary_master;
extern uint8_t ide_secondary_slave;
extern uint32_t ide_primary_master_sectors;
extern uint32_t ide_primary_slave_sectors;
extern uint32_t ide_secondary_master_sectors;
extern uint32_t ide_secondary_slave_sectors;

#endif
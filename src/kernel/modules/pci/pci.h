#ifndef PCI_H
#define PCI_H

#include <stdint.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

#define PCI_VENDOR_INVALID 0xFFFF

#define PCI_CLASS_MASS_STORAGE 0x01
#define PCI_CLASS_BRIDGE       0x06
#define PCI_CLASS_SERIAL       0x07
#define PCI_CLASS_DISPLAY      0x03
#define PCI_CLASS_NETWORK      0x02
#define PCI_CLASS_MULTIMEDIA   0x04
#define PCI_CLASS_SIMPLE_COMM  0x07

#define PCI_SUBCLASS_NVME      0x08
#define PCI_SUBCLASS_SATA_AHCI 0x06
#define PCI_SUBCLASS_IDE       0x01
#define PCI_SUBCLASS_VGA       0x00

typedef struct {
    uint8_t  bus;
    uint8_t  device;
    uint8_t  function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t  class;
    uint8_t  subclass;
    uint8_t  prog_if;
    uint8_t  header_type;
    uint32_t bar0;
    uint32_t bar1;
    uint32_t bar2;
    uint32_t bar3;
    uint32_t bar4;
    uint32_t bar5;
    uint16_t command;
    uint16_t status_reg;
    uint8_t  irq_line;
    uint8_t  irq_pin;
    uint8_t  revision_id;
    uint8_t  cache_line_size;
    uint8_t  latency_timer;
    uint8_t  bist;
} pci_device_t;

#define PCI_MAX_DEVICES 64

extern pci_device_t pci_devices[PCI_MAX_DEVICES];
extern int pci_device_count;

void pci_init(void);
uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
void pci_config_write(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);
const char *pci_class_name(uint8_t class_code, uint8_t subclass, uint8_t prog_if);

#endif

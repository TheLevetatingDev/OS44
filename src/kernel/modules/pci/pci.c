#include "pci.h"
#include "../interrupts/interrupts.h"
#include <stdint.h>

static void pcer_putc(char c) { while ((inb(0x3F8 + 5) & 0x20) == 0); outb(0x3F8, c); }
static void pcer(const char *s) { while (*s) { if (*s=='\n') pcer_putc('\r'); pcer_putc(*s++); } }

pci_device_t pci_devices[PCI_MAX_DEVICES];
int pci_device_count = 0;

static void pci_check_device(uint8_t bus, uint8_t device, uint8_t function);

uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t addr = (uint32_t)0x80000000
        | ((uint32_t)bus  << 16)
        | ((uint32_t)device << 11)
        | ((uint32_t)function << 8)
        | (offset & 0xFC);
    outl(PCI_CONFIG_ADDRESS, addr);
    return inl(PCI_CONFIG_DATA);
}

void pci_config_write(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value) {
    uint32_t addr = (uint32_t)0x80000000
        | ((uint32_t)bus  << 16)
        | ((uint32_t)device << 11)
        | ((uint32_t)function << 8)
        | (offset & 0xFC);
    outl(PCI_CONFIG_ADDRESS, addr);
    outl(PCI_CONFIG_DATA, value);
}

static void pci_read_device(pci_device_t *dev) {
    uint32_t reg0 = pci_config_read(dev->bus, dev->device, dev->function, 0x00);
    uint32_t reg8 = pci_config_read(dev->bus, dev->device, dev->function, 0x08);
    uint32_t regC = pci_config_read(dev->bus, dev->device, dev->function, 0x0C);
    uint32_t reg10 = pci_config_read(dev->bus, dev->device, dev->function, 0x10);
    uint32_t reg14 = pci_config_read(dev->bus, dev->device, dev->function, 0x14);
    uint32_t reg18 = pci_config_read(dev->bus, dev->device, dev->function, 0x18);
    uint32_t reg1C = pci_config_read(dev->bus, dev->device, dev->function, 0x1C);
    uint32_t reg20 = pci_config_read(dev->bus, dev->device, dev->function, 0x20);
    uint32_t reg24 = pci_config_read(dev->bus, dev->device, dev->function, 0x24);

    dev->vendor_id     = (uint16_t)(reg0 & 0xFFFF);
    dev->device_id     = (uint16_t)((reg0 >> 16) & 0xFFFF);
    dev->command       = (uint16_t)(regC & 0xFFFF);
    dev->status_reg    = (uint16_t)((regC >> 16) & 0xFFFF);
    dev->revision_id   = (uint8_t)(reg8 & 0xFF);
    dev->prog_if       = (uint8_t)((reg8 >> 8) & 0xFF);
    dev->subclass      = (uint8_t)((reg8 >> 16) & 0xFF);
    dev->class         = (uint8_t)((reg8 >> 24) & 0xFF);
    dev->cache_line_size = (uint8_t)(regC & 0xFF);
    dev->latency_timer   = (uint8_t)((regC >> 8) & 0xFF);
    dev->header_type     = (uint8_t)((regC >> 16) & 0xFF);
    dev->bist            = (uint8_t)((regC >> 24) & 0xFF);
    dev->bar0 = reg10;
    dev->bar1 = reg14;
    dev->bar2 = reg18;
    dev->bar3 = reg1C;
    dev->bar4 = reg20;
    dev->bar5 = reg24;

    uint32_t reg3C = pci_config_read(dev->bus, dev->device, dev->function, 0x3C);
    dev->irq_line = (uint8_t)(reg3C & 0xFF);
    dev->irq_pin  = (uint8_t)((reg3C >> 8) & 0xFF);
}

static void pci_check_device(uint8_t bus, uint8_t device, uint8_t function) {
    uint32_t reg0 = pci_config_read(bus, device, function, 0x00);
    uint16_t vendor_id = (uint16_t)(reg0 & 0xFFFF);
    if (vendor_id == PCI_VENDOR_INVALID) return;

    if (pci_device_count >= PCI_MAX_DEVICES) return;

    pci_device_t *dev = &pci_devices[pci_device_count++];
    dev->bus = bus;
    dev->device = device;
    dev->function = function;
    pci_read_device(dev);

    uint32_t regC = pci_config_read(bus, device, function, 0x0C);
    uint8_t header_type = (uint8_t)((regC >> 16) & 0xFF);
    if ((header_type & 0x7F) == 0) {
        // Check for multifunction device
    }
}

static void pci_check_bus(uint8_t bus) {
    for (uint8_t device = 0; device < 32; device++) {
        uint32_t reg0 = pci_config_read(bus, device, 0, 0x00);
        uint16_t vendor_id = (uint16_t)(reg0 & 0xFFFF);
        if (vendor_id == PCI_VENDOR_INVALID) continue;

        pci_check_device(bus, device, 0);

        uint32_t regC = pci_config_read(bus, device, 0, 0x0C);
        uint8_t header_type = (uint8_t)((regC >> 16) & 0xFF);
        if (header_type & 0x80) {
            for (uint8_t function = 1; function < 8; function++) {
                reg0 = pci_config_read(bus, device, function, 0x00);
                vendor_id = (uint16_t)(reg0 & 0xFFFF);
                if (vendor_id != PCI_VENDOR_INVALID) {
                    pci_check_device(bus, device, function);
                }
            }
        }
    }
}

void pci_init(void) {
    pci_device_count = 0;
    pcer("[PCI] start\n");

    pci_check_bus(0);
    pcer("[PCI] bus 0 done\n");

    uint32_t reg0 = pci_config_read(0, 0, 0, 0x0C);
    uint8_t ht = (uint8_t)((reg0 >> 16) & 0xFF);
    if ((ht & 0x80) == 0) {
        // Single function host bridge
    } else {
        for (uint8_t bus = 1; bus < 255; bus++) {
            pci_check_bus(bus);
        }
    }
    pcer("[PCI] done, count=");
    { char b[16]; int bi=0; uint32_t v=pci_device_count; if(!v){b[bi++]='0';} else {char t[16];int tj=0;while(v){t[tj++]='0'+v%10;v/=10;}while(tj)b[bi++]=t[--tj];} b[bi]=0; pcer(b); pcer("\n"); }
}

const char *pci_class_name(uint8_t class_code, uint8_t subclass, uint8_t prog_if) {
    (void)prog_if;
    switch (class_code) {
        case 0x00: return subclass == 0x00 ? "Legacy Device" : "VGA-Compatible";
        case 0x01:
            switch (subclass) {
                case 0x00: return "SCSI";
                case 0x01: return "IDE";
                case 0x06: return "SATA (AHCI)";
                case 0x08: return "NVMe";
                default:   return "Mass Storage";
            }
        case 0x02: return "Network";
        case 0x03:
            switch (subclass) {
                case 0x00: return "VGA";
                case 0x80: return "Display";
                default:   return "Display";
            }
        case 0x04: return "Multimedia";
        case 0x05: return "Memory";
        case 0x06:
            switch (subclass) {
                case 0x00: return "Host Bridge";
                case 0x01: return "ISA Bridge";
                case 0x04: return "PCI-to-PCI Bridge";
                case 0x80: return "Bridge";
                default:   return "Bridge";
            }
        case 0x07:
            switch (subclass) {
                case 0x00: return "Serial (UART)";
                case 0x01: return "Parallel";
                case 0x03: return "Multiport Serial";
                default:   return "Communication";
            }
        case 0x08:
            switch (subclass) {
                case 0x00: return "PIC";
                case 0x01: return "DMA";
                case 0x02: return "Timer";
                case 0x03: return "RTC";
                case 0x04: return "PCI Hotplug";
                case 0x05: return "SD Host";
                case 0x06: return "IOMMU";
                default:   return "System";
            }
        case 0x09: return "Input";
        case 0x0C:
            switch (subclass) {
                case 0x03: return "USB";
                default:   return "Serial Bus";
            }
        case 0xFF: return "Unassigned";
        default:   return "Unknown";
    }
}

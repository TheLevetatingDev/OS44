#ifndef BOOTINFO_H
#define BOOTINFO_H

#include <stdint.h>

// Must match bootloader.c BootInfo
typedef struct {
    uint32_t magic;
    void    *framebuffer;
    uint64_t fb_width;
    uint64_t fb_height;
    uint64_t fb_pitch;
    uint32_t fb_format;
    void    *mmap;
    uint64_t mmap_size;
    uint64_t mmap_desc_size;
} BootInfo;

#define BOOTINFO_MAGIC 0xB007B007

// EFI_GRAPHICS_PIXEL_FORMAT (GOP)
#define FB_FORMAT_RGB 0
#define FB_FORMAT_BGR 1

#endif

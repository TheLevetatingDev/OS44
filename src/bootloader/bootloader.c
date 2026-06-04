#include <stdint.h>

// ============================================================
// Minimal UEFI types and protocol definitions (no gnu-efi)
// ============================================================

typedef uint8_t   UINT8;
typedef uint16_t  UINT16;
typedef uint32_t  UINT32;
typedef uint64_t  UINT64;
typedef int64_t   INT64;
typedef uintptr_t UINTN;
typedef void* EFI_HANDLE;
typedef UINT64    EFI_STATUS;
typedef UINT64    EFI_PHYSICAL_ADDRESS;

#define EFI_SUCCESS              0ULL
#define EFI_ERROR(x)             ((x) & (1ULL << 63))
#define EFI_LOAD_ERROR           (1ULL | (1ULL << 63))

#define EFI_MEMORY_TYPE_LOADER_DATA  2

// UEFI calling convention on x86_64 is MS ABI (same as Windows)
#define EFIAPI __attribute__((ms_abi))
// Kernel calling convention is System V AMD64 ABI (Linux default)
#define SYSVAPI __attribute__((sysv_abi))

// --- UEFI GUIDs ---
typedef struct { UINT32 a; UINT16 b, c; UINT8 d[8]; } EFI_GUID;

#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID \
    {0x9042a9de,0x23dc,0x4a38,{0x96,0xfb,0x7a,0xde,0xd0,0x80,0x51,0x6a}}
#define EFI_LOADED_IMAGE_PROTOCOL_GUID \
    {0x5b1b31a1,0x9562,0x11d2,{0x8e,0x3f,0x00,0xa0,0xc9,0x69,0x72,0x3b}}
#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
    {0x964e5b22,0x6459,0x11d2,{0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b}}

// --- Memory map ---
typedef struct {
    UINT32 Type;
    UINT32 Pad;
    EFI_PHYSICAL_ADDRESS PhysicalStart;
    UINT64 VirtualStart;
    UINT64 NumberOfPages;
    UINT64 Attribute;
} EFI_MEMORY_DESCRIPTOR;

// --- GOP ---
typedef struct {
    UINT32 RedMask, GreenMask, BlueMask, ReservedMask;
} EFI_PIXEL_BITMASK;

typedef enum { PixelRedGreenBlueReserved8BitPerColor,
               PixelBlueGreenRedReserved8BitPerColor,
               PixelBitMask, PixelBltOnly, PixelFormatMax } EFI_GRAPHICS_PIXEL_FORMAT;

typedef struct {
    UINT32 Version;
    UINT32 HorizontalResolution;
    UINT32 VerticalResolution;
    EFI_GRAPHICS_PIXEL_FORMAT PixelFormat;
    EFI_PIXEL_BITMASK PixelInformation;
    UINT32 PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct {
    UINT32 MaxMode, Mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    UINTN SizeOfInfo;
    EFI_PHYSICAL_ADDRESS FrameBufferBase;
    UINTN FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

typedef struct _EFI_GRAPHICS_OUTPUT_PROTOCOL {
    void *QueryMode, *SetMode, *Blt;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *Mode;
} EFI_GRAPHICS_OUTPUT_PROTOCOL;

// --- File protocol ---
#define EFI_FILE_MODE_READ 1ULL

typedef struct _EFI_FILE_PROTOCOL EFI_FILE_PROTOCOL;
typedef struct _EFI_FILE_PROTOCOL {
    UINT64 Revision;
    EFIAPI EFI_STATUS (*Open)(EFI_FILE_PROTOCOL*, EFI_FILE_PROTOCOL**, UINT16*, UINT64, UINT64);
    EFIAPI EFI_STATUS (*Close)(EFI_FILE_PROTOCOL*);
    void *Delete;
    EFIAPI EFI_STATUS (*Read)(EFI_FILE_PROTOCOL*, UINTN*, void*);
    void *Write;
    EFIAPI EFI_STATUS (*GetPosition)(EFI_FILE_PROTOCOL*, UINT64*);
    EFIAPI EFI_STATUS (*SetPosition)(EFI_FILE_PROTOCOL*, UINT64);
    void *GetInfo, *SetInfo, *Flush;
} EFI_FILE_PROTOCOL;

typedef struct {
    UINT64 Revision;
    EFIAPI EFI_STATUS (*OpenVolume)(void*, EFI_FILE_PROTOCOL**);
} EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

// --- Loaded image ---
typedef struct {
    UINT32 Revision;
    EFI_HANDLE ParentHandle;
    void *SystemTable;
    EFI_HANDLE DeviceHandle;
} EFI_LOADED_IMAGE_PROTOCOL;

// --- Boot services (Aligned strictly to UEFI Spec) ---
typedef struct {
    UINT8 Pad[24]; // EFI_TABLE_HEADER
    void *RaiseTPL, *RestoreTPL;
    EFIAPI EFI_STATUS (*AllocatePages)(UINT32, UINT32, UINTN, EFI_PHYSICAL_ADDRESS*);
    void *FreePages;
    EFIAPI EFI_STATUS (*GetMemoryMap)(UINTN*, EFI_MEMORY_DESCRIPTOR*, UINTN*, UINTN*, UINT32*);
    EFIAPI EFI_STATUS (*AllocatePool)(UINT32, UINTN, void**);
    void *FreePool;
    void *CreateEvent, *SetTimer, *WaitForEvent, *SignalEvent, *CloseEvent, *CheckEvent;
    void *InstallProtocolInterface, *ReinstallProtocolInterface, *UninstallProtocolInterface;
    EFIAPI EFI_STATUS (*HandleProtocol)(EFI_HANDLE, EFI_GUID*, void**);
    void *Reserved;
    void *RegisterProtocolNotify, *LocateHandle;
    void *LocateDevicePath, *InstallConfigurationTable;
    void *LoadImage, *StartImage, *Exit, *UnloadImage;
    EFIAPI EFI_STATUS (*ExitBootServices)(EFI_HANDLE, UINTN);
    void *GetNextMonotonicCount, *Stall, *SetWatchdogTimer;
    void *ConnectController, *DisconnectController;
    void *OpenProtocol, *CloseProtocol, *OpenProtocolInformation;
    void *ProtocolsPerHandle, *LocateHandleBuffer;
    EFIAPI EFI_STATUS (*LocateProtocol)(EFI_GUID*, void*, void**);
} EFI_BOOT_SERVICES;

// --- System table ---
typedef struct {
    UINT8 Hdr[24];
    UINT16 *FirmwareVendor;
    UINT32 FirmwareRevision;
    UINT32 Pad; // 32-bit padding for explicit 64-bit boundary matching
    EFI_HANDLE ConsoleInHandle;
    void *ConIn;
    EFI_HANDLE ConsoleOutHandle;
    void *ConOut;
    EFI_HANDLE StandardErrorHandle;
    void *StdErr;
    void *RuntimeServices;
    EFI_BOOT_SERVICES *BootServices;
} EFI_SYSTEM_TABLE;

// ============================================================
// ELF64
// ============================================================

#define PT_LOAD 1

typedef struct {
    UINT8  e_ident[16];
    UINT16 e_type, e_machine;
    UINT32 e_version;
    UINT64 e_entry, e_phoff, e_shoff;
    UINT32 e_flags;
    UINT16 e_ehsize, e_phentsize, e_phnum;
    UINT16 e_shentsize, e_shnum, e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    UINT32 p_type, p_flags;
    UINT64 p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_align;
} Elf64_Phdr;

// ============================================================
// BootInfo passed to kernel
// ============================================================

typedef struct {
    UINT32 magic;
    void  *framebuffer;
    UINT64 fb_width, fb_height, fb_pitch;
    UINT32 fb_format;
    void  *mmap;
    UINT64 mmap_size, mmap_desc_size;
} BootInfo;

// ============================================================
// Helpers
// ============================================================

static EFI_BOOT_SERVICES *BS;

static void memset_local(void *dst, UINT8 val, UINTN len) {
    UINT8 *p = dst;
    while (len--) *p++ = val;
}

// Simple print via ConOut
static void print(EFI_SYSTEM_TABLE *st, UINT16 *msg) {
    typedef EFI_STATUS (EFIAPI *EFI_OUTPUT_STRING)(void*, UINT16*);
    EFI_OUTPUT_STRING OutputString = *(EFI_OUTPUT_STRING*)((UINT8 *)st->ConOut + sizeof(void*));
    OutputString(st->ConOut, msg);
}

// ============================================================
// Kernel loader
// ============================================================

static EFI_STATUS load_kernel(EFI_FILE_PROTOCOL *root, UINT64 *entry_out) {
    EFI_FILE_PROTOCOL *kfile;
    UINT16 kname[] = {'k','e','r','n','e','l','.','e','l','f',0};

    EFI_STATUS s = root->Open(root, &kfile, kname, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(s)) return s;

    Elf64_Ehdr ehdr;
    UINTN sz = sizeof(ehdr);
    kfile->Read(kfile, &sz, &ehdr);

    if (ehdr.e_ident[0] != 0x7f || ehdr.e_ident[1] != 'E' ||
        ehdr.e_ident[2] != 'L'  || ehdr.e_ident[3] != 'F')
        return EFI_LOAD_ERROR;

    Elf64_Phdr phdrs[16];
    UINTN phsz = ehdr.e_phnum * sizeof(Elf64_Phdr);
    kfile->SetPosition(kfile, ehdr.e_phoff);
    kfile->Read(kfile, &phsz, phdrs);

    for (int i = 0; i < ehdr.e_phnum; i++) {
        if (phdrs[i].p_type != PT_LOAD) continue;
        UINTN pages = (phdrs[i].p_memsz + 0xFFF) / 0x1000;
        EFI_PHYSICAL_ADDRESS seg = phdrs[i].p_vaddr; // Map to virtual target destination
        
        BS->AllocatePages(1 /*AllocateAddress*/, 2 /*EfiLoaderData*/, pages, &seg);
        memset_local((void *)seg, 0, phdrs[i].p_memsz);
        
        UINTN fsz = phdrs[i].p_filesz;
        kfile->SetPosition(kfile, phdrs[i].p_offset);
        kfile->Read(kfile, &fsz, (void *)seg);
    }

    *entry_out = ehdr.e_entry;
    kfile->Close(kfile);
    return EFI_SUCCESS;
}

// ============================================================
// Entry point
// ============================================================

EFIAPI EFI_STATUS EfiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    BS = SystemTable->BootServices;

    print(SystemTable, (UINT16 *)u"=== OS44 Bootloader ===\r\n");

    // --- GOP framebuffer ---
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    if (EFI_ERROR(BS->LocateProtocol(&gop_guid, 0, (void **)&gop))) {
        print(SystemTable, (UINT16 *)u"ERROR: No GOP\r\n");
        while(1);
    }

    // --- Open ESP filesystem ---
    EFI_GUID li_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_LOADED_IMAGE_PROTOCOL *li;
    BS->HandleProtocol(ImageHandle, &li_guid, (void **)&li);

    EFI_GUID fs_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;
    BS->HandleProtocol(li->DeviceHandle, &fs_guid, (void **)&fs);

    EFI_FILE_PROTOCOL *root;
    fs->OpenVolume(fs, &root);

    // --- Load kernel ---
    UINT64 kernel_entry = 0;
    if (EFI_ERROR(load_kernel(root, &kernel_entry))) {
        print(SystemTable, (UINT16 *)u"ERROR: kernel load failed\r\n");
        while(1);
    }
    print(SystemTable, (UINT16 *)u"Kernel loaded!\r\n");

    // --- Get memory map & exit boot services ---
    UINTN mmap_size = 0, map_key, desc_size;
    UINT32 desc_ver;
    EFI_MEMORY_DESCRIPTOR *mmap = 0;

    // Fetch required map buffer size
    BS->GetMemoryMap(&mmap_size, mmap, &map_key, &desc_size, &desc_ver);
    
    // BARE METAL FIX: Add a generous 4KB cushion for fragmented desktop firmware pools
    mmap_size += 4096; 
    BS->AllocatePool(2, mmap_size, (void **)&mmap);
    BS->GetMemoryMap(&mmap_size, mmap, &map_key, &desc_size, &desc_ver);

    // --- Build BootInfo ---
    BootInfo *info;
    BS->AllocatePool(2, sizeof(BootInfo), (void **)&info);
    info->magic          = 0xB007B007;
    info->framebuffer    = (void *)gop->Mode->FrameBufferBase;
    info->fb_width       = gop->Mode->Info->HorizontalResolution;
    info->fb_height      = gop->Mode->Info->VerticalResolution;
    info->fb_pitch       = gop->Mode->Info->PixelsPerScanLine * 4;
    info->fb_format      = gop->Mode->Info->PixelFormat;
    info->mmap           = mmap;
    info->mmap_size      = mmap_size;
    info->mmap_desc_size = desc_size;

    // BARE METAL FIX: Robust retry loop for handling background firmware timers
    EFI_STATUS es = BS->ExitBootServices(ImageHandle, map_key);
    if (EFI_ERROR(es)) {
        BS->GetMemoryMap(&mmap_size, mmap, &map_key, &desc_size, &desc_ver);
        es = BS->ExitBootServices(ImageHandle, map_key);
        if (EFI_ERROR(es)) {
            print(SystemTable, (UINT16 *)u"ERROR: Failed to exit boot services.\r\n");
            while(1);
        }
    }

    // --- Jump to kernel ---
    // BARE METAL FIX: Explicit System V ABI calling convention cast for kernel handoff
    SYSVAPI void (*kernel_main)(BootInfo *) = (SYSVAPI void (*)(BootInfo *))kernel_entry;
    kernel_main(info);

    while(1) __asm__("hlt");
    return EFI_SUCCESS;
}
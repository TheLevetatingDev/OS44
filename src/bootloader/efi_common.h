#ifndef EFI_COMMON_H
#define EFI_COMMON_H

#include <stdint.h>
#include <stddef.h>

// UEFI calling convention on x86_64 is MS ABI (same as Windows)
#define EFIAPI __attribute__((ms_abi))

typedef uint8_t   UINT8;
typedef uint16_t  UINT16;
typedef uint32_t  UINT32;
typedef uint64_t  UINT64;
typedef int64_t   INT64;
typedef uintptr_t UINTN;

#define EFI_SUCCESS              0ULL
#define EFI_ERROR(x)             ((x) & (1ULL << 63))
#define EFI_LOAD_ERROR           (1ULL | (1ULL << 63))

typedef uint16_t CHAR16;
typedef uint64_t EFI_STATUS;
typedef void *EFI_HANDLE;
typedef uint64_t EFI_PHYSICAL_ADDRESS;

typedef struct {
    uint64_t Signature;
    uint32_t Revision;
    uint32_t HeaderSize;
    uint32_t CRC32;
    uint32_t Reserved;
} EFI_TABLE_HEADER;

typedef struct _EFI_BOOT_SERVICES {
    EFI_TABLE_HEADER Hdr;
    void *RaiseTPL, *RestoreTPL;
    EFI_STATUS (*AllocatePages)(uint32_t, uint32_t, uint64_t, uint64_t*);
    void *FreePages;
    EFI_STATUS (*GetMemoryMap)(uint64_t*, void*, uint64_t*, uint64_t*, uint32_t*);
    EFI_STATUS (*AllocatePool)(uint32_t, uint64_t, void**);
    void *FreePool;
    void *CreateEvent, *SetTimer, *WaitForEvent, *SignalEvent, *CloseEvent, *CheckEvent;
    void *InstallProtocolInterface, *ReinstallProtocolInterface, *UninstallProtocolInterface;
    EFI_STATUS (*HandleProtocol)(void*, void*, void**);
    void *Reserved;
    void *RegisterProtocolNotify, *LocateHandle;
    void *LocateDevicePath, *InstallConfigurationTable;
    void *LoadImage, *StartImage, *Exit, *UnloadImage;
    EFI_STATUS (*ExitBootServices)(void*, uint64_t);
    void *GetNextMonotonicCount, *Stall, *SetWatchdogTimer;
    void *ConnectController, *DisconnectController;
    void *OpenProtocol, *CloseProtocol, *OpenProtocolInformation;
    void *ProtocolsPerHandle, *LocateHandleBuffer;
    EFI_STATUS (*LocateProtocol)(void*, void*, void**);
} EFI_BOOT_SERVICES;

struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef struct _EFI_SYSTEM_TABLE {
    EFI_TABLE_HEADER Hdr;
    CHAR16 *FirmwareVendor;
    uint32_t FirmwareRevision;
    uint32_t Pad; // Padding to align subsequent 64-bit pointers
    void *ConsoleInHandle;
    void *ConIn;
    void *ConsoleOutHandle;
    struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
    void *StandardErrorHandle;
    void *StdErr;
    void *RuntimeServices;
    struct _EFI_BOOT_SERVICES *BootServices;
} EFI_SYSTEM_TABLE;

#endif // EFI_COMMON_H

#include "efi_common.h"

// UEFI entry point
EFIAPI EFI_STATUS EfiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    // UEFI uses UTF-16 strings (L"...")
    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16 *)L"Hello from minimal EFI!\n");
    
    // Infinite loop to keep the message on screen
    while (1) {
        __asm__("hlt");
    }
    
    return 0;
}
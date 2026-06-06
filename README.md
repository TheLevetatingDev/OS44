# OS44: A Minimal x86_64 Operating System

OS44 is a hobby operating system project designed for x86_64 architecture. It features a custom UEFI bootloader written in C and a modular kernel also written in C, built using `zig cc`. The project aims to provide a clear and educational codebase for understanding fundamental operating system concepts.

## Features

*   **UEFI Bootloader:** A custom C-based bootloader (`BOOTX64.EFI`) responsible for initializing the system in a UEFI environment.
*   **ELF64 Kernel Loading:** The bootloader loads the `kernel.elf` executable, parsing its ELF64 structure and setting up its memory.
*   **Framebuffer Graphics:** Utilizes the UEFI Graphics Output Protocol (GOP) to provide basic graphical output with double buffering for smoother UI rendering.
*   **Physical Memory Management (PMM):** Manages the allocation and deallocation of physical memory frames.
*   **Interrupt Handling:** Basic setup for hardware interrupts, including a timer and keyboard support.
*   **Simple Shell:** A command-line interface within the kernel for basic interaction.
*   **System Information:** Gathers and displays basic system details.
*   **Modular Kernel Design:** The kernel is structured into various modules for better organization and maintainability (e.g., framebuffer, memory, interrupts, keyboard, timer, shell).

## Directory Structure

*   `iso/`: Contains the final bootable image (`BOOTX64.EFI`) and the `kernel.elf` to be placed in the EFI partition.
*   `src/`: Contains all source code.
    *   `bootloader/`: Source code for the UEFI bootloader (`bootloader.c`, `efi_common.h`, `Makefile`).
    *   `kernel/`: Main kernel source code (`kernel.c`, `linker.ld`, `Makefile`).
        *   `modules/`: Individual kernel modules:
            *   `framebuffer/`: Framebuffer initialization and drawing functions.
            *   `interrupts/`: Interrupt Descriptor Table (IDT) setup and Interrupt Service Routines (ISRs).
            *   `keyboard/`: Keyboard driver.
            *   `mem/`: Memory management utilities (PMM, paging).
            *   `shell/`: Simple command-line interpreter.
            *   `startup_panel/`: Initial graphical display during boot.
            *   `sysinfo/`: Functions to retrieve and display system information.
            *   `timer/`: Programmable Interval Timer (PIT) driver.

## Dependencies

This project relies on `zig cc` as the compiler and `nasm` for assembly.
For Arch Linux, you can install the necessary tools using `pacman` and `yay`:

```bash
sudo pacman -S base-devel nasm qemu ovmf mingw-w64-gcc gnu-efi zig
paru -S x86_64-elf-gcc x86_64-elf-binutils
```
**Note:** `mingw-w64-gcc` and `gnu-efi` are primarily for `x86_64-elf-gcc` and `x86_64-elf-binutils` to work with UEFI. The actual compilation uses `zig cc`.

## Build & Run

To build the bootloader and kernel, and then create the ISO image:

```bash
make
```

To run the OS in QEMU:

```bash
make run
```

This will launch QEMU with the built OS, utilizing OVMF (Open Virtual Machine Firmware) for UEFI booting.

## Future Plans

*   Implement Virtual Memory Management (VMM).
*   Develop Fat32 FS Support.
*   Explore multitasking and process management.
*   Enhance the shell with more commands and features.
*   Add more hardware drivers.
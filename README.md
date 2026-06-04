# OS44

A minimal x86_64 OS with a custom UEFI bootloader and C kernel.

## Install dependencies (Arch Linux)

```bash
sudo pacman -S base-devel nasm qemu ovmf mingw-w64-gcc gnu-efi
yay -S x86_64-elf-gcc x86_64-elf-binutils
```

## Build & run in QEMU

```bash
make run
```

## YAY; THIS THING FINALLY WORKING
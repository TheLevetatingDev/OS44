OVMF    := /usr/share/edk2/x64/OVMF.4m.fd
ISO_DIR := iso

ISO_NAME := OS44_$(shell date +%Y%m%d_%H%M%S).iso

.PHONY: all iso run clean

all: iso

# ── Sub-project builds ────────────────────────────────────────────────────────
src/bootloader/BOOTX64.EFI:
	$(MAKE) -C src/bootloader

src/kernel/kernel.elf:
	$(MAKE) -C src/kernel

# ── Bootable ISO ──────────────────────────────────────────────────────────────
#
# Uses xorriso only — no mtools, mkfs.fat, or root required.
# xorriso builds a proper El Torito EFI boot entry (MBR + GPT hybrid),
# which makes the resulting image directly flashable to a USB stick with dd.
#
# Install dependency (once):  sudo apt install xorriso
#
# Flash to USB:
#   sudo dd if=OS44_*.iso of=/dev/sdX bs=4M status=progress && sync
#   (find your USB device with: lsblk)
#
iso: src/bootloader/BOOTX64.EFI src/kernel/kernel.elf
	@echo ">>> Building bootable ISO: $(ISO_NAME)"
	@mkdir -p $(ISO_DIR)
	@mkdir -p /tmp/efi_mnt
	@dd if=/dev/zero of=$(ISO_DIR)/efi.img bs=1M count=64
	@mkfs.vfat -F 32 $(ISO_DIR)/efi.img
	@sudo mount -o loop $(ISO_DIR)/efi.img /tmp/efi_mnt
	@sudo mkdir -p /tmp/efi_mnt/EFI/BOOT
	@sudo cp src/bootloader/BOOTX64.EFI /tmp/efi_mnt/EFI/BOOT/BOOTX64.EFI
	@sudo cp src/kernel/kernel.elf /tmp/efi_mnt/kernel.elf
	@sudo umount /tmp/efi_mnt
	@# No need to copy kernel.elf to ISO_DIR now, as it's in efi.img
	xorriso -as mkisofs \
	    -o $(ISO_NAME) \
	    -e efi.img \
	    -no-emul-boot \
	    -isohybrid-gpt-basdat \
	    $(ISO_DIR)
	@echo ""
	@echo ">>> Done: $(ISO_NAME)"
	@echo ">>> Flash: sudo dd if=$(ISO_NAME) of=/dev/sdX bs=4M status=progress && sync"
	@echo ">>> Fix: echo "fix" | sudo parted /dev/sdX ---pretend-input-tty print"

# ── QEMU run with the generated ISO ───────────────────────────────────────────
runiso: iso
	@echo ">>> Running QEMU with ISO: $(shell ls -t OS44_*.iso | head -1)"
	qemu-system-x86_64 \
	    -bios $(OVMF) \
	    -cdrom $(shell ls -t OS44_*.iso | head -1) \
	    -m 256M \
	    -serial stdio

# ── QEMU test run (using iso directory directly) ──────────────────────────────
run: src/bootloader/BOOTX64.EFI src/kernel/kernel.elf
	@mkdir -p $(ISO_DIR)/EFI/BOOT
	@cp src/bootloader/BOOTX64.EFI $(ISO_DIR)/EFI/BOOT/
	@cp src/kernel/kernel.elf       $(ISO_DIR)/
	qemu-system-x86_64 \
	    -bios $(OVMF) \
	    -drive file=fat:rw:$(ISO_DIR) \
	    -m 256M \
	    -serial stdio

# ── Clean ─────────────────────────────────────────────────────────────────────
clean:
	$(MAKE) -C src/bootloader clean
	$(MAKE) -C src/kernel clean
	rm -rf $(ISO_DIR) OS44_*.iso

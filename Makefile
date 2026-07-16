OVMF_VARS_LOCAL := vars.fd
ISO_DIR         := iso
IDE_IMG         := ide.img
IDE_IMG_SIZE    := 20M

ISO_NAME := OS44_$(shell date +%Y%m%d_%H%M%S).iso

# ── Dynamic OVMF Detection ────────────────────────────────────────────────────
# This searches common Linux system paths for the 4M (or standard) OVMF files.
OVMF_PATHS := \
  /usr/share/OVMF \
  /usr/share/ovmf \
  /usr/share/ovmf/x64 \
  /usr/share/edk2/ovmf \
  /usr/share/edk2-ovmf/x64

# Locate OVMF_CODE (preferring the 4M version if available)
OVMF_CODE := $(firstword $(wildcard \
  $(foreach path,$(OVMF_PATHS),$(path)/OVMF_CODE_4M.fd) \
  $(foreach path,$(OVMF_PATHS),$(path)/OVMF_CODE.fd) \
  $(foreach path,$(OVMF_PATHS),$(path)/ovmf_code.fd) \
))

# Locate the matching template OVMF_VARS
OVMF_VARS_TEMPLATE := $(firstword $(wildcard \
  $(foreach path,$(OVMF_PATHS),$(path)/OVMF_VARS_4M.fd) \
  $(foreach path,$(OVMF_PATHS),$(path)/OVMF_VARS.fd) \
  $(foreach path,$(OVMF_PATHS),$(path)/ovmf_vars.fd) \
))

.PHONY: all iso run clean runiso run-ide runiso-ide help

# ── Help ────────────────────────────────────────────────────────────────────────
help:
	@echo "Available targets:"
	@echo "  make           - Build ISO (default)"
	@echo "  make iso       - Build bootable ISO"
	@echo "  make run       - Run in QEMU (virtual FAT directory)"
	@echo "  make runiso    - Run in QEMU (from ISO)"
	@echo "  make run-ide   - Run in QEMU with 20MB IDE drive (ide.img)"
	@echo "  make runiso-ide - Run in QEMU with ISO + 20MB IDE drive"
	@echo "  make run IDE=1 - Run with IDE drive (alias for run-ide)"
	@echo "  make runiso IDE=1 - Run ISO with IDE drive (alias for runiso-ide)"
	@echo "  make clean     - Clean build artifacts"
	@echo "  make help      - Show this help"

all: iso

# ── Helper to restore vars.fd ─────────────────────────────────────────────────
# This rule will run automatically if vars.fd is missing
$(OVMF_VARS_LOCAL):
	@if [ -z "$(OVMF_VARS_TEMPLATE)" ]; then \
		echo "ERROR: Could not find system OVMF_VARS template on your system!"; \
		echo "Please install 'ovmf' or 'edk2-ovmf' package."; \
		exit 1; \
	fi
	@echo ">>> System OVMF Code found at: $(OVMF_CODE)"
	@echo ">>> System OVMF Vars template found at: $(OVMF_VARS_TEMPLATE)"
	@echo ">>> Copying system template to local $(OVMF_VARS_LOCAL)..."
	@cp $(OVMF_VARS_TEMPLATE) $(OVMF_VARS_LOCAL)

# ── Sub-project builds ────────────────────────────────────────────────────────
src/bootloader/BOOTX64.EFI:
	$(MAKE) -C src/bootloader

src/kernel/kernel.elf:
	$(MAKE) -C src/kernel

# ── Bootable ISO ──────────────────────────────────────────────────────────────
iso: src/bootloader/BOOTX64.EFI src/kernel/kernel.elf
	@echo ">>> Building bootable ISO: $(ISO_NAME)"
	@mkdir -p $(ISO_DIR)
	@rm -rf staging
	
	@# Setup Staging Directory Structure
	@mkdir -p staging/EFI/BOOT
	@cp src/bootloader/BOOTX64.EFI staging/EFI/BOOT/BOOTX64.EFI
	@cp src/kernel/kernel.elf staging/kernel.elf
	
	@# Create a clean 64MB FAT32 image
	@dd if=/dev/zero of=staging/efi.img bs=1M count=64 status=none
	@mkfs.vfat -F 32 staging/efi.img > /dev/null
	
	@# Use mtools to structure the FAT image
	@mmd -i staging/efi.img ::/EFI
	@mmd -i staging/efi.img ::/EFI/BOOT
	@mcopy -i staging/efi.img src/bootloader/BOOTX64.EFI ::/EFI/BOOT/BOOTX64.EFI
	@mcopy -i staging/efi.img src/kernel/kernel.elf ::/kernel.elf
	
	@# Generate the hybrid ISO containing BOTH staging files and efi.img
	xorriso -as mkisofs \
		-o $(ISO_NAME) \
		-e efi.img \
		-no-emul-boot \
		-isohybrid-gpt-basdat \
		staging
	
	@# Clean up staging directory
	@rm -rf staging
	@echo ""
	@echo ">>> Done: $(ISO_NAME)"
	@echo ">>> Flash: sudo dd if=$(ISO_NAME) of=/dev/sdX bs=4M status=progress && sync"

# ── QEMU run with the generated ISO ───────────────────────────────────────────
runiso: iso $(OVMF_VARS_LOCAL)
ifeq ($(IDE),1)
	@$(MAKE) ide.img
endif
	@if [ -z "$(OVMF_CODE)" ]; then \
		echo "ERROR: Could not find system OVMF_CODE.fd!"; \
		exit 1; \
	fi
	@echo ">>> Running QEMU with ISO: $(shell ls -t OS44_*.iso | head -1)"
ifeq ($(IDE),1)
	qemu-system-x86_64 \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-drive if=pflash,format=raw,file=$(OVMF_VARS_LOCAL) \
		-cdrom $(shell ls -t OS44_*.iso | head -1) \
		-drive if=ide,format=raw,file=ide.img \
		-m 256M \
		-boot order=d \
		-net none \
		-serial stdio
else
	qemu-system-x86_64 \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-drive if=pflash,format=raw,file=$(OVMF_VARS_LOCAL) \
		-cdrom $(shell ls -t OS44_*.iso | head -1) \
		-m 256M \
		-boot d \
		-net none \
		-serial stdio
endif

# ── QEMU test run (using virtual FAT directory directly) ──────────────────────
run: src/bootloader/BOOTX64.EFI src/kernel/kernel.elf $(OVMF_VARS_LOCAL)
ifeq ($(IDE),1)
	@$(MAKE) ide.img
endif
	@if [ -z "$(OVMF_CODE)" ]; then \
		echo "ERROR: Could not find system OVMF_CODE.fd!"; \
		exit 1; \
	fi
	@mkdir -p $(ISO_DIR)/EFI/BOOT
	@cp src/bootloader/BOOTX64.EFI $(ISO_DIR)/EFI/BOOT/
	@cp src/kernel/kernel.elf       $(ISO_DIR)/
ifeq ($(IDE),1)
	@echo ">>> Running QEMU with IDE drive (ide.img) + virtual FAT"
	qemu-system-x86_64 \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-drive if=pflash,format=raw,file=$(OVMF_VARS_LOCAL) \
		-drive format=raw,file=fat:rw:$(ISO_DIR) \
		-drive if=ide,format=raw,file=ide.img \
		-m 256M \
		-serial stdio
else
	qemu-system-x86_64 \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-drive if=pflash,format=raw,file=$(OVMF_VARS_LOCAL) \
		-drive format=raw,file=fat:rw:$(ISO_DIR) \
		-m 256M \
		-serial stdio
endif

# ── Clean ─────────────────────────────────────────────────────────────────────
clean:
	$(MAKE) -C src/bootloader clean
	$(MAKE) -C src/kernel clean
	rm -rf $(ISO_DIR) OS44_*.iso staging $(OVMF_VARS_LOCAL) ide.img

# ── IDE 20MB disk image ───────────────────────────────────────────────────────
ide.img:
	@echo ">>> Creating 20MB IDE disk image: ide.img"
	@dd if=/dev/zero of=ide.img bs=1M count=20 status=none

# ── QEMU run with IDE drive (using virtual FAT dir + IDE image) ───────────────
run-ide: ide.img src/bootloader/BOOTX64.EFI src/kernel/kernel.elf $(OVMF_VARS_LOCAL)
	@if [ -z "$(OVMF_CODE)" ]; then \
		echo "ERROR: Could not find system OVMF_CODE.fd!"; \
		exit 1; \
	fi
	@mkdir -p $(ISO_DIR)/EFI/BOOT
	@cp src/bootloader/BOOTX64.EFI $(ISO_DIR)/EFI/BOOT/
	@cp src/kernel/kernel.elf       $(ISO_DIR)/
	@echo ">>> Running QEMU with IDE drive (ide.img) + virtual FAT"
	qemu-system-x86_64 \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-drive if=pflash,format=raw,file=$(OVMF_VARS_LOCAL) \
		-drive format=raw,file=fat:rw:$(ISO_DIR) \
		-drive if=ide,format=raw,file=ide.img \
		-m 256M \
		-serial stdio

# ── QEMU run with ISO + IDE drive ─────────────────────────────────────────────
runiso-ide: ide.img iso $(OVMF_VARS_LOCAL)
	@if [ -z "$(OVMF_CODE)" ]; then \
		echo "ERROR: Could not find system OVMF_CODE.fd!"; \
		exit 1; \
	fi
	@echo ">>> Running QEMU with ISO: $(shell ls -t OS44_*.iso | head -1) + IDE drive (ide.img)"
	qemu-system-x86_64 \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-drive if=pflash,format=raw,file=$(OVMF_VARS_LOCAL) \
		-cdrom $(shell ls -t OS44_*.iso | head -1) \
		-drive if=ide,format=raw,file=ide.img \
		-m 256M \
		-boot order=d \
		-net none \
		-serial stdio

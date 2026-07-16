OVMF_VARS_LOCAL := vars.fd
ISO_DIR         := iso

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

.PHONY: all iso run clean runiso

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
	@if [ -z "$(OVMF_CODE)" ]; then \
		echo "ERROR: Could not find system OVMF_CODE.fd!"; \
		exit 1; \
	fi
	@echo ">>> Running QEMU with ISO: $(shell ls -t OS44_*.iso | head -1)"
	qemu-system-x86_64 \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-drive if=pflash,format=raw,file=$(OVMF_VARS_LOCAL) \
		-cdrom $(shell ls -t OS44_*.iso | head -1) \
		-m 256M \
		-boot d \
		-net none \
		-serial stdio

# ── QEMU test run (using virtual FAT directory directly) ──────────────────────
run: src/bootloader/BOOTX64.EFI src/kernel/kernel.elf $(OVMF_VARS_LOCAL)
	@if [ -z "$(OVMF_CODE)" ]; then \
		echo "ERROR: Could not find system OVMF_CODE.fd!"; \
		exit 1; \
	fi
	@mkdir -p $(ISO_DIR)/EFI/BOOT
	@cp src/bootloader/BOOTX64.EFI $(ISO_DIR)/EFI/BOOT/
	@cp src/kernel/kernel.elf       $(ISO_DIR)/
	qemu-system-x86_64 \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-drive if=pflash,format=raw,file=$(OVMF_VARS_LOCAL) \
		-drive format=raw,file=fat:rw:$(ISO_DIR) \
		-m 256M \
		-serial stdio

# ── Clean ─────────────────────────────────────────────────────────────────────
clean:
	$(MAKE) -C src/bootloader clean
	$(MAKE) -C src/kernel clean
	rm -rf $(ISO_DIR) OS44_*.iso staging $(OVMF_VARS_LOCAL)

OVMF_VARS_LOCAL := vars.fd
ISO_DIR         := iso
IDE_IMG         := ide.img
IDE_IMG_SIZE    := 20M

ISO_NAME := OS44_$(shell date +%Y%m%d_%H%M%S).iso

# ── Dynamic OVMF Detection ────────────────────────────────────────────────────
OVMF_PATHS := \
  . \
  /opt/homebrew/share/qemu \
  /usr/local/share/qemu \
  /usr/share/OVMF \
  /usr/share/ovmf \
  /usr/share/ovmf/x64 \
  /usr/share/edk2/ovmf \
  /usr/share/edk2-ovmf/x64

OVMF_CODE := $(firstword $(wildcard \
  OVMF_CODE.fd \
  $(foreach path,$(OVMF_PATHS),$(path)/OVMF_CODE_4M.fd) \
  $(foreach path,$(OVMF_PATHS),$(path)/OVMF_CODE.fd) \
  $(foreach path,$(OVMF_PATHS),$(path)/ovmf_code.fd) \
))

OVMF_VARS_TEMPLATE := $(firstword $(wildcard \
  vars.fd \
  $(foreach path,$(OVMF_PATHS),$(path)/OVMF_VARS_4M.fd) \
  $(foreach path,$(OVMF_PATHS),$(path)/OVMF_VARS.fd) \
  $(foreach path,$(OVMF_PATHS),$(path)/ovmf_vars.fd) \
  $(foreach path,$(OVMF_PATHS),$(path)/edk2-x86_64-vars.fd) \
  $(foreach path,$(OVMF_PATHS),$(path)/edk2-i386-vars.fd) \
))

.PHONY: all iso run clean runiso run-ide runiso-ide help ensure-ovmf-code

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

# ── Ensure OVMF_CODE.fd exists ─────────────────────────────────────────────────
ensure-ovmf-code:
	@if [ -z "$(OVMF_CODE)" ] || [ $$(wc -c < OVMF_CODE.fd 2>/dev/null || echo 0) -lt 100000 ]; then \
		echo ">>> OVMF_CODE valid binary not found. Downloading..."; \
		curl -L -o OVMF_CODE.fd.bz2 "https://raw.githubusercontent.com/qemu/qemu/master/pc-bios/edk2-x86_64-code.fd.bz2"; \
		bunzip2 -f OVMF_CODE.fd.bz2; \
	fi

# ── Helper to restore/download vars.fd ────────────────────────────────────────
$(OVMF_VARS_LOCAL):
	@if [ -n "$(OVMF_VARS_TEMPLATE)" ] && [ "$(OVMF_VARS_TEMPLATE)" != "$(OVMF_VARS_LOCAL)" ]; then \
		echo ">>> System OVMF Code found at: $(OVMF_CODE)"; \
		echo ">>> System OVMF Vars template found at: $(OVMF_VARS_TEMPLATE)"; \
		echo ">>> Copying system template to local $(OVMF_VARS_LOCAL)..."; \
		cp $(OVMF_VARS_TEMPLATE) $(OVMF_VARS_LOCAL); \
	elif [ ! -f "$(OVMF_VARS_LOCAL)" ] || [ $$(wc -c < $(OVMF_VARS_LOCAL) 2>/dev/null || echo 0) -lt 50000 ]; then \
		echo ">>> OVMF_VARS template valid binary not found. Downloading..."; \
		curl -L -o vars.fd.bz2 "https://raw.githubusercontent.com/qemu/qemu/master/pc-bios/edk2-i386-vars.fd.bz2"; \
		bunzip2 -f vars.fd.bz2; \
	fi

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
	@mkdir -p staging/EFI/BOOT
	@cp src/bootloader/BOOTX64.EFI staging/EFI/BOOT/BOOTX64.EFI
	@cp src/kernel/kernel.elf staging/kernel.elf
	@dd if=/dev/zero of=staging/efi.img bs=1M count=64 status=none
	@mkfs.vfat -F 32 staging/efi.img > /dev/null
	@mmd -i staging/efi.img ::/EFI
	@mmd -i staging/efi.img ::/EFI/BOOT
	@mcopy -i staging/efi.img src/bootloader/BOOTX64.EFI ::/EFI/BOOT/BOOTX64.EFI
	@mcopy -i staging/efi.img src/kernel/kernel.elf ::/kernel.elf
	xorriso -as mkisofs \
		-o $(ISO_NAME) \
		-e efi.img \
		-no-emul-boot \
		-isohybrid-gpt-basdat \
		staging
	@rm -rf staging
	@echo ""
	@echo ">>> Done: $(ISO_NAME)"
	@echo ">>> Flash: sudo dd if=$(ISO_NAME) of=/dev/sdX bs=4M status=progress && sync"

# ── QEMU run with the generated ISO ───────────────────────────────────────────
runiso: iso ensure-ovmf-code $(OVMF_VARS_LOCAL)
ifeq ($(IDE),1)
	@$(MAKE) ide.img
endif
	@echo ">>> Running QEMU with ISO: $$(ls -t OS44_*.iso | head -1)"
ifeq ($(IDE),1)
	qemu-system-x86_64 \
		-drive if=pflash,format=raw,readonly=on,file=$$(if [ -n "$(OVMF_CODE)" ]; then echo "$(OVMF_CODE)"; else echo "OVMF_CODE.fd"; fi) \
		-drive if=pflash,format=raw,file=$(OVMF_VARS_LOCAL) \
		-cdrom $$(ls -t OS44_*.iso | head -1) \
		-drive if=ide,format=raw,file=ide.img \
		-m 256M \
		-boot order=d \
		-net none \
		-serial stdio
else
	qemu-system-x86_64 \
		-drive if=pflash,format=raw,readonly=on,file=$$(if [ -n "$(OVMF_CODE)" ]; then echo "$(OVMF_CODE)"; else echo "OVMF_CODE.fd"; fi) \
		-drive if=pflash,format=raw,file=$(OVMF_VARS_LOCAL) \
		-cdrom $$(ls -t OS44_*.iso | head -1) \
		-m 256M \
		-boot d \
		-net none \
		-serial stdio
endif

# ── QEMU test run (using virtual FAT directory directly) ──────────────────────
run: src/bootloader/BOOTX64.EFI src/kernel/kernel.elf ensure-ovmf-code $(OVMF_VARS_LOCAL)
ifeq ($(IDE),1)
	@$(MAKE) ide.img
endif
	@mkdir -p $(ISO_DIR)/EFI/BOOT
	@cp src/bootloader/BOOTX64.EFI $(ISO_DIR)/EFI/BOOT/
	@cp src/kernel/kernel.elf       $(ISO_DIR)/
ifeq ($(IDE),1)
	@echo ">>> Running QEMU with IDE drive (ide.img) + virtual FAT"
	qemu-system-x86_64 \
		-drive if=pflash,format=raw,readonly=on,file=$$(if [ -n "$(OVMF_CODE)" ]; then echo "$(OVMF_CODE)"; else echo "OVMF_CODE.fd"; fi) \
		-drive if=pflash,format=raw,file=$(OVMF_VARS_LOCAL) \
		-drive format=raw,file=fat:rw:$(ISO_DIR) \
		-drive if=ide,format=raw,file=ide.img \
		-m 256M \
		-serial stdio
else
	qemu-system-x86_64 \
		-drive if=pflash,format=raw,readonly=on,file=$$(if [ -n "$(OVMF_CODE)" ]; then echo "$(OVMF_CODE)"; else echo "OVMF_CODE.fd"; fi) \
		-drive if=pflash,format=raw,file=$(OVMF_VARS_LOCAL) \
		-drive format=raw,file=fat:rw:$(ISO_DIR) \
		-m 256M \
		-serial stdio
endif

# ── Clean ─────────────────────────────────────────────────────────────────────
clean:
	$(MAKE) -C src/bootloader clean
	$(MAKE) -C src/kernel clean
	rm -rf $(ISO_DIR) OS44_*.iso staging $(OVMF_VARS_LOCAL) OVMF_CODE.fd ide.img

# ── IDE 20MB disk image ───────────────────────────────────────────────────────
ide.img:
	@echo ">>> Creating 20MB IDE disk image: ide.img"
	@dd if=/dev/zero of=ide.img bs=1M count=20 status=none

# ── QEMU run with IDE drive (using virtual FAT dir + IDE image) ───────────────
run-ide: ide.img src/bootloader/BOOTX64.EFI src/kernel/kernel.elf ensure-ovmf-code $(OVMF_VARS_LOCAL)
	@mkdir -p $(ISO_DIR)/EFI/BOOT
	@cp src/bootloader/BOOTX64.EFI $(ISO_DIR)/EFI/BOOT/
	@cp src/kernel/kernel.elf       $(ISO_DIR)/
	@echo ">>> Running QEMU with IDE drive (ide.img) + virtual FAT"
	qemu-system-x86_64 \
		-drive if=pflash,format=raw,readonly=on,file=$$(if [ -n "$(OVMF_CODE)" ]; then echo "$(OVMF_CODE)"; else echo "OVMF_CODE.fd"; fi) \
		-drive if=pflash,format=raw,file=$(OVMF_VARS_LOCAL) \
		-drive format=raw,file=fat:rw:$(ISO_DIR) \
		-drive if=ide,format=raw,file=ide.img \
		-m 256M \
		-serial stdio

# ── QEMU run with ISO + IDE drive ─────────────────────────────────────────────
runiso-ide: ide.img iso ensure-ovmf-code $(OVMF_VARS_LOCAL)
	@echo ">>> Running QEMU with ISO: $$(ls -t OS44_*.iso | head -1) + IDE drive (ide.img)"
	qemu-system-x86_64 \
		-drive if=pflash,format=raw,readonly=on,file=$$(if [ -n "$(OVMF_CODE)" ]; then echo "$(OVMF_CODE)"; else echo "OVMF_CODE.fd"; fi) \
		-drive if=pflash,format=raw,file=$(OVMF_VARS_LOCAL) \
		-cdrom $$(ls -t OS44_*.iso | head -1) \
		-drive if=ide,format=raw,file=ide.img \
		-m 256M \
		-boot order=d \
		-net none \
		-serial stdio

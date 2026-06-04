OVMF := /usr/share/edk2/x64/OVMF.4m.fd

.PHONY: all clean run

all:
	$(MAKE) -C src/bootloader
	$(MAKE) -C src/kernel
	$(MAKE) disk.img

disk.img:
	dd if=/dev/zero of=$@ bs=1M count=64
	parted $@ --script mklabel gpt
	parted $@ --script mkpart ESP fat32 2048s 100%
	parted $@ --script set 1 esp on
	sudo losetup -D
	sudo losetup -Pf --show $@ > /tmp/loopdev
	sudo mkfs.fat -F32 $$(cat /tmp/loopdev)p1
	mkdir -p /tmp/esp
	sudo mount $$(cat /tmp/loopdev)p1 /tmp/esp
	sudo mkdir -p /tmp/esp/EFI/BOOT
	sudo cp src/bootloader/BOOTX64.EFI /tmp/esp/EFI/BOOT/BOOTX64.EFI
	sudo cp src/kernel/kernel.elf /tmp/esp/kernel.elf
	sudo umount /tmp/esp
	sudo losetup -d $$(cat /tmp/loopdev)

run: all
	qemu-system-x86_64 \
		-bios $(OVMF) \
		-drive format=raw,file=disk.img \
		-m 256M \
		-serial stdio

clean:
	$(MAKE) -C src/bootloader clean
	$(MAKE) -C src/kernel clean
	rm -f disk.img
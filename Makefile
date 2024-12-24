CC = gcc
CFLAGS = -m32 -ffreestanding -nostdlib -nostartfiles -nodefaultlibs
AS = nasm
ASFLAGS = -f bin

BOOTLOADER = boot.asm
MULTIBOOT_HEADER = multiboot_header.asm
KERNEL = kernel.c
LINKER = link.ld

BOOTLOADER_BIN = boot.bin
KERNEL_BIN = kernel.bin
ISO = mykernel.iso

all: $(ISO)

$(BOOTLOADER_BIN): $(BOOTLOADER)
	$(AS) $(ASFLAGS) -o $@ $<

$(KERNEL_BIN): $(KERNEL) $(MULTIBOOT_HEADER)
	$(AS) -f elf $(MULTIBOOT_HEADER) -o multiboot_header.o
	$(CC) $(CFLAGS) -c $(KERNEL) -o kernel.o
	ld -m elf_i386 -T $(LINKER) -o $@ multiboot_header.o kernel.o

$(ISO): $(BOOTLOADER_BIN) $(KERNEL_BIN)
	mkdir -p iso/boot/grub
	cp $(BOOTLOADER_BIN) iso/boot/
	cp $(KERNEL_BIN) iso/boot/
	echo "set timeout=0" > iso/boot/grub/grub.cfg
	echo "set default=0" >> iso/boot/grub/grub.cfg
	echo "menuentry 'My Kernel' {" >> iso/boot/grub/grub.cfg
	echo "  multiboot /boot/kernel.bin" >> iso/boot/grub/grub.cfg
	echo "  boot" >> iso/boot/grub/grub.cfg
	echo " }" >> iso/boot/grub/grub.cfg
	grub-mkrescue -o $@ iso

clean:
	rm -rf *.o *.bin iso $(ISO)

re: clean all

run:
	qemu-system-i386 -cdrom mykernel.iso

xorriso:
	xorriso -indev mykernel.iso -ls /boot/grub/


.PHONY: all clean re run

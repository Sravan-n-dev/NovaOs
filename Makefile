GPPFLAGS = -m32 -ffreestanding -fno-use-cxa-atexit -fno-exceptions -fno-rtti \
           -fno-leading-underscore -fno-stack-protector -fno-pie -Wall -Wextra \
           -Isrc
ASFLAGS  = --32
LDFLAGS  = -melf_i386 -T linker.ld

objects = build/loader.o \
          build/interruptstubs.o \
          build/gdt.o \
          build/port.o \
          build/interrupts.o \
          build/console.o \
          build/keyboard.o \
          build/timer.o \
          build/multitasking.o \
          build/shell.o \
          build/kernel.o

build/%.o: src/%.cpp
	@mkdir -p build
	g++ $(GPPFLAGS) -c $< -o $@

build/%.o: src/%.s
	@mkdir -p build
	as $(ASFLAGS) $< -o $@

build/novaos.bin: linker.ld $(objects)
	ld $(LDFLAGS) -o $@ $(objects)
	@grub-file --is-x86-multiboot $@ && echo "Multiboot check: OK"

novaos.iso: build/novaos.bin grub/grub.cfg
	mkdir -p iso/boot/grub
	cp build/novaos.bin iso/boot/
	cp grub/grub.cfg iso/boot/grub/
	grub-mkrescue -o $@ iso 2>/dev/null

run: novaos.iso
	qemu-system-i386 -cdrom novaos.iso -m 64M

debug: novaos.iso
	qemu-system-i386 -cdrom novaos.iso -m 64M -s -S &
	gdb build/novaos.bin -ex "target remote localhost:1234"

clean:
	rm -rf build iso novaos.iso

.PHONY: run debug clean

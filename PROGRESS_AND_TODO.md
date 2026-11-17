# NovaOS - Progress, Architecture & Development Roadmap

## Project Overview
NovaOS is an educational 32-bit x86 operating system developed following the incremental methodology of the "Write your own Operating System" (WYOOS) tutorial series by TheSalarKhan / Nanobyte.

---

## 1. What Has Been Completed So Far

### Stage 1: Development Environment & Toolchain Specification
- **Target Architecture:** x86 32-bit protected mode (`-m32`, `i386`).
- **Toolchain:** GCC/G++ (`-ffreestanding`, `-fno-exceptions`, `-fno-rtti`, `-fno-pie`), GNU `as` (`--32`), GNU `ld` (`-melf_i386`), `qemu-system-i386`, `grub-mkrescue`, `xorriso`.
- **Build Automation:** `Makefile` supporting compilation, linking, ISO image creation, QEMU launching, and GDB debugging.

### Stage 2: Bootloader, Linker Script & Minimal Kernel
- **Linker Script (`linker.ld`):** 
  - Loads kernel at physical address `0x00100000` (1 MiB mark).
  - Positions Multiboot header in the first 8 KiB.
  - Collects `.text`, `.rodata`, `.init_array` / `.ctors` for global C++ constructors, `.data`, and `.bss`.
- **Boot Entry (`src/loader.s`):**
  - Multiboot 1 compliant magic (`0x1BADB002`), flags (`0x03` = page aligned modules + memory map), checksum.
  - Allocates 2 MiB kernel stack in `.bss`.
  - Sets up `%esp`, passes Multiboot info pointer (`%ebx`) and magic (`%eax`).
  - Calls `callConstructors` and jumps to `kernelMain`.
- **Types Definition (`src/types.h`):** Standard fixed-width integer types (`uint8_t`, `uint16_t`, `uint32_t`, `int8_t`, `int16_t`, `int32_t`).
- **Kernel Entry (`src/kernel.cpp`):**
  - C++ static constructor invoker (`callConstructors`).
  - Minimal VGA text buffer writer (`0xB8000`) verifying Multiboot magic (`0x2BADB002`).
- **GRUB Configuration (`grub/grub.cfg`):** Menu configuration for loading `novaos.bin` directly with Multiboot protocol.

### Stage 3: Hardware Port Communication (`src/port.h`, `src/port.cpp`)
- Built the WYOOS hardware I/O abstraction hierarchy:
  - `Port`: Base class holding port address.
  - `Port8Bit`: 8-bit `inb` and `outb` with inline assembly.
  - `Port8BitSlow`: 8-bit I/O with small bus cycle delay (`outb %al, $0x80`).
  - `Port16Bit`: 16-bit `inw` and `outw`.
  - `Port32Bit`: 32-bit `inl` and `outl`.

### Stage 4: Global Descriptor Table (GDT) (`src/gdt.h`, `src/gdt.cpp`)
- Fully implemented the GDT segment structure according to x86 specifications:
  - `SegmentDescriptor` inner class packing base, limit (with byte or 4 KiB page granularity), and access/type flags.
  - Null descriptor, unused descriptor, kernel code segment descriptor (`0x9A`, ring 0, 4 GiB flat address space), kernel data segment descriptor (`0x92`, ring 0, 4 GiB flat address space).
  - Installs GDT via `lgdt` instruction.
  - Exposes `CodeSegmentSelector()` and `DataSegmentSelector()` offsets.

### Stage 5: Interrupt Descriptor Table (IDT), 8259 PIC Remap & Interrupt Stubs (`src/interrupts.h`, `src/interrupts.cpp`, `src/interruptstubs.s`)
- Defined WYOOS interrupt architecture:
  - `GateDescriptor` structure packed for x86 IDT entries (handler low/high 16 bits, code segment selector, access flags).
  - `InterruptHandler` base class for pluggable peripheral device handlers.
  - `InterruptManager`: Manages 256 IDT gates, master PIC (ports `0x20`/`0x21`) and slave PIC (ports `0xA0`/`0xA1`) remapping, and dispatching.
  - `src/interruptstubs.s`: Assembly stubs for CPU exceptions 0x00-0x13, hardware IRQs 0x00-0x0F, register saving (`pusha`, segment registers), C++ invocation (`HandleInterrupt`), and stack pointer reloading for preemptive multitasking.
  - `src/interrupts.cpp`: Complete 8259 PIC cascading initialization (ICW1-4), IDT gate registration, `lidt` activation, EOI signaling, and dispatch loop.
  - `src/kernel.cpp`: Initialized GDT, IDT (offset 0x20), and enabled hardware interrupts with `sti`.

---

## 2. What Needs to Be Done (Next Steps)

To complete NovaOS strictly following the prompt specifications and WYOOS architecture, implement the following components in order:

### 1. Stage 6: Keyboard Driver
- **`src/keyboard.h` & `src/keyboard.cpp`:**
  - Class `KeyboardDriver` inheriting from `InterruptHandler` (`interruptNumber = 0x21`).
  - Connects to PS/2 data port `0x60` and command/status port `0x64`.
  - Translates scan codes (Scancode Set 1) to ASCII characters.
  - Handles Shift, CapsLock, Enter, Backspace.
  - Buffers characters or forwards them to active Console/Shell.

### 2. Stage 7: VGA Console & Interactive Command Shell
- **`src/console.h` & `src/console.cpp`:**
  - VGA text driver for 80x25 character grid at `0xB8000`.
  - Cursor tracking, screen scrolling on line 25 overflow, color attributes.
  - `printf`-like formatting or simple string/hex/integer output.
- **`src/shell.h` & `src/shell.cpp`:**
  - Interactive prompt `novaos> `.
  - Built-in commands: `help`, `clear`, `tasks`, `mem`, `sysinfo`, `reboot`.

### 3. Stage 8: Timer Interrupt (PIT 8253/8254)
- **`src/timer.h` & `src/timer.cpp`:**
  - Channel 0 on port `0x40`, Mode/Command register on port `0x43`.
  - Set PIT frequency (e.g. 100 Hz or 1000 Hz).
  - Track kernel uptime / tick count.

### 4. Stage 9 & 10: Task Management & Preemptive Round Robin Multitasking
- **`src/multitasking.h` & `src/multitasking.cpp`:**
  - `CPUState` struct matching the stack layout saved by `interruptstubs.s` (`eax`, `ebx`, `ecx`, `edx`, `esi`, `edi`, `ebp`, `esp`, `eip`, `cs`, `eflags`, etc.).
  - `Task` class containing unique Task ID, 4 KiB stack, state (`RUNNING`, `READY`, `TERMINATED`), and pointer to saved `CPUState`.
  - `TaskManager`: Holds list of tasks, current task index.
  - In timer interrupt handler (`HandleInterruptRequest0x00`), call `taskManager.Schedule(esp)`:
    - Save current `esp` into `tasks[currentTask]->cpuState`.
    - Select next `READY` task in round-robin sequence.
    - Return next task's `cpuState` address as the new `esp`.
    - `interruptstubs.s` restores this `esp` and executes `iret`, switching tasks seamlessly!

### 5. Stage 11: Dynamic Memory Management (Heap Allocation)
- **`src/memorymanagement.h` & `src/memorymanagement.cpp`:**
  - Simple memory manager tracking heap starting at e.g. 4 MiB or end of kernel binary.
  - `MemoryChunk` header structure: `size`, `allocated` (bool), `prev`, `next`.
  - Implement `malloc(size_t)` / `free(void*)`.
  - Overload global `operator new`, `operator new[]`, `operator delete`, `operator delete[]`.

### 6. Stage 12: System Monitor
- Display live metrics queried from real kernel structures:
  - Kernel memory usage and free heap bytes from `MemoryManager`.
  - Active and ready task counts, current task ID from `TaskManager`.
  - Uptime and timer ticks.

### 7. Stage 13: Academic Documentation & Viva Preparation
- Comprehensive documentation report covering:
  - Abstract, Problem Statement, Objectives, Architectural Blueprint.
  - Hardware Abstractions vs Theory (x86 ring model, segmentation vs paging, interrupt vectors, PIC vs APIC).
  - Viva Q&A covering boot protocols, GDT, IDT, context switching, and heap management.

---

## 3. How to Build & Test

### Ubuntu Prerequisites
```bash
sudo apt update
sudo apt install -y build-essential g++-multilib make grub-pc-bin grub-common xorriso mtools qemu-system-x86 gdb
```

### Build Commands
```bash
make novaos.iso     # Compiles all sources, checks Multiboot header, builds bootable ISO
make run            # Runs NovaOS inside QEMU with 64MB RAM
make debug          # Starts QEMU with GDB stub on port 1234
make clean          # Cleans build artifacts
```

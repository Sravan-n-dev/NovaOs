# NovaOS - Progress, Architecture & Development Roadmap

## Project Overview
NovaOS is an educational 32-bit x86 operating system developed following the incremental methodology of the "Write your own Operating System" (WYOOS) tutorial series by TheSalarKhan / Nanobyte.

---

## 1. What Has Been Completed & Pushed to GitHub

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
  - Initializes GDT, IDT, PIC, Keyboard, PIT Timer, TaskManager, 3 automatic demo tasks, and launches the Interactive Shell.
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
  - Corrected C++ Itanium ABI name mangling: `_ZN16InterruptManager19HandleException0x##Ev` and `_ZN16InterruptManager26HandleInterruptRequest0x##Ev`.
  - `src/interrupts.cpp`: Complete 8259 PIC cascading initialization (ICW1-4), IDT gate registration, `lidt` activation, EOI signaling, and dispatch loop.

### Stage 6: PS/2 Keyboard Driver (`src/keyboard.h`, `src/keyboard.cpp`)
- Class `KeyboardDriver` inheriting from `InterruptHandler` (`interruptNumber = 0x21` / IRQ1).
- Communicates through PS/2 data port `0x60` and command/status port `0x64`.
- Circular FIFO buffer (`KEYBUF_SIZE = 128`).
- Full Scancode Set 1 translation table to ASCII with Shift and CapsLock state handling.
- Exposes non-blocking `getChar()` and `hasChar()` interfaces.

### Stage 7: VGA Console & Interactive Command Shell (`src/console.h`, `src/console.cpp`, `src/shell.h`, `src/shell.cpp`)
- `Console`: 80x25 VGA text mode driver (`0xB8000`), hardware cursor tracking via ports `0x3D4`/`0x3D5`, vertical scrolling, color attribute management, formatted integer and hex printing.
- `Shell`: Interactive shell running at `novaos> ` prompt:
  - `help`: Lists all supported commands.
  - `info`: Displays OS version, architecture, 32-bit protected mode, scheduler, timer, keyboard status.
  - `tasks`: Queries real kernel data structures to show task IDs, states (`RUNNING`, `READY`, `TERMINATED`), total task count, and context-switch count.
  - `start`: Manually creates and enqueues a new independent kernel task with its own 4 KiB stack.
  - `uptime`: Calculates and displays system uptime from real PIT ticks.
  - `clear`: Clears the display screen.

### Stage 8: PIT Timer (`src/timer.h`, `src/timer.cpp`)
- Configured 8254 Programmable Interval Timer (Channel 0 port `0x40`, Mode/Command port `0x43`) in Mode 2 (Rate Generator) at 100 Hz (`PIT_DIVISOR = 11932`).
- Handles IRQ0 interrupts, tracks uptime ticks, and triggers the `TaskManager::Schedule` context switch.

### Stage 9 & 10: Preemptive Multitasking & Context Switching (`src/multitasking.h`, `src/multitasking.cpp`)
- **`CPUState` Struct**: Exact byte-for-byte stack layout matching `interruptstubs.s` (`gs`, `fs`, `es`, `ds`, `edi`, `esi`, `ebp`, `pusha_esp`, `ebx`, `edx`, `ecx`, `eax`, `error`, `eip`, `cs`, `eflags`).
- **`Task` TCB**: Statically allocated task slots (up to 16 tasks), each with an isolated 4 KiB kernel stack (`TASK_STACK_SIZE = 4096`), task ID, and state.
- **Round-Robin Scheduling**:
  - Task 0 represents the boot/shell execution context.
  - 3 demo tasks (Task A, Task B, Task C) are automatically initialized and started at boot with visible progress spinners.
  - Additional tasks can be manually created via the shell `start` command.
  - Every PIT tick interrupts the running task, saves its stack frame, selects the next `READY` task, and swaps `%esp` via `%eax` return in `interruptstubs.s`.

---

## 2. Academic Project Report & Theoretical Alignment

### Theoretical Concepts Demonstrated
1. **CPU Modes & Privilege Levels:** x86 Protected Mode execution with Ring 0 flat memory segments via GDT.
2. **Interrupt-Driven Architecture:** Hardware interrupts handled through 8259 PIC and x86 IDT gate descriptors rather than busy-polling.
3. **Preemptive Scheduling vs Cooperative Scheduling:** Tasks do not need to yield; hardware PIT timer interrupts forcibly preempt CPU execution and transfer control to another task.
4. **Context Switching Mechanism:** Preservation of general-purpose and segment registers onto the task's private stack frame, exchanging the stack pointer `%esp`, and restoring context via `iret`.

---

## 3. How to Build & Run (Ubuntu 24.04)

```bash
# Install dependencies
sudo apt update
sudo apt install -y build-essential g++-multilib make grub-pc-bin grub-common xorriso mtools qemu-system-x86 gdb

# Build bootable ISO
make clean
make novaos.iso

# Launch in QEMU
make run

# Debug with GDB
make debug
```

# NovaOS - Implementation Status & Development Roadmap

## Project Overview
NovaOS is an educational 32-bit x86 operating system developed following the incremental methodology of the "Write your own Operating System" (WYOOS) tutorial series.

**Project Scope:** A minimal x86 OS in 32-bit protected mode featuring genuine kernel-level preemptive Round-Robin multitasking, hardware timer-driven context switching, and an interactive command shell. (Memory management subsystems, paging, user-mode rings, and filesystems are intentionally out of scope).

---

## 1. Component Status Matrix

| Component | Module Files | Source Code Status | QEMU Runtime Status | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **Bootloader & Entry** | `src/loader.s`, `linker.ld` | **Implemented** | *Pending Ubuntu test* | Multiboot 1 compliant, 2MB stack, 1MB load address |
| **Kernel Entry** | `src/kernel.cpp`, `grub/grub.cfg` | **Implemented** | *Pending Ubuntu test* | Global ctors, VGA verify, subsystem bringup |
| **GDT** | `src/gdt.h`, `src/gdt.cpp` | **Implemented** | *Pending Ubuntu test* | Flat Ring 0 model: 4GB Code (`0x10`), 4GB Data (`0x18`) |
| **Hardware Port I/O** | `src/port.h`, `src/port.cpp` | **Implemented** | *Pending Ubuntu test* | 8/16/32-bit I/O with inline `in`/`out` assembly |
| **Interrupt Infrastructure** | `src/interrupts.h`, `src/interrupts.cpp`, `src/interruptstubs.s` | **Implemented** | *Pending Ubuntu test* | Dual 8259 PIC remapped (`0x20`-`0x2F`), 256-gate IDT, context stubs with `movl %eax, %esp` |
| **VGA Console Driver** | `src/console.h`, `src/console.cpp` | **Implemented** | *Pending Ubuntu test* | 80x25 text buffer (`0xB8000`), scrolling, cursor I/O (`0x3D4`/`0x3D5`), colors |
| **PS/2 Keyboard Driver** | `src/keyboard.h`, `src/keyboard.cpp` | **Implemented** | *Pending Ubuntu test* | IRQ1 handler, 128-byte circular FIFO, Scancode Set 1 decoding, Shift/CapsLock |
| **PIT Timer Driver** | `src/timer.h`, `src/timer.cpp` | **Implemented** | *Pending Ubuntu test* | Mode 2 Rate Generator (`0x34`), 100 Hz frequency (`div=11932`), drives scheduler |
| **Preemptive Multitasking** | `src/multitasking.h`, `src/multitasking.cpp` | **Implemented** | *Pending Ubuntu test* | True CPU context switching via timer ISR, static TCB slots, isolated 4KB stacks |
| **Task Creation Race Fix** | `src/multitasking.cpp` | **Implemented & Fixed** | *Pending Ubuntu test* | `cli`/`popfl` lock during creation; `TASK_READY` set only after `cpuState` is fully formed |
| **Interactive Shell** | `src/shell.h`, `src/shell.cpp` | **Implemented** | *Pending Ubuntu test* | Runs as Task 0; commands: `help`, `info`, `tasks`, `start`, `uptime`, `clear` |
| **Makefile Integration** | `Makefile` | **Implemented & Verified** | *Pending Ubuntu test* | Compiles all 10 object files, builds `novaos.iso`, launches QEMU |

---

## 2. Details of Implemented Source Code

### A. Preemptive Context-Switching Mechanism
- **Interrupt Stack Preservation:** When IRQ0 triggers, the hardware pushes `EFLAGS`, `CS`, `EIP`. The assembly stub (`src/interruptstubs.s`) pushes an error code placeholder, general registers via `pusha`, and segment registers `DS`, `ES`, `FS`, `GS`.
- **Stack Pointer Exchange:** The C++ timer handler invokes `TaskManager::Schedule(esp)`. The scheduler saves the outgoing task's `%esp`, circular-scans for the next `TASK_READY` task, marks it `TASK_RUNNING`, and returns its saved `%esp` in register `%eax`.
- **Restoration:** The stub executes `movl %eax, %esp`, pointing the CPU to the new task's private stack, then pops segments, `popa`, and executes `iret`.

### B. Concurrency Safety & Race Condition Prevention
- Inside `TaskManager::CreateTask(TaskFunction entryPoint)`:
  1. Interrupts are disabled via `pushfl; popl %0; cli` so timer ticks cannot preempt task initialization.
  2. The task's 4 KiB private stack and `CPUState` struct are constructed with `EFLAGS=0x202` (IF=1) and `CS=0x10`.
  3. `tasks[slot].cpuState` is assigned.
  4. Only after the context is 100% complete is `tasks[slot].state = TASK_READY` published.
  5. The previous interrupt flags are restored via `pushl %0; popfl`.

### C. Timer Configuration Consistency
- PIT Channel 0 (Port `0x40`) is configured with control word `0x34` (`0b00110100`):
  - Channel 0, Lobyte/Hibyte access, Mode 2 (Rate Generator), 16-bit binary counter.
  - Generates a periodic interrupt pulse every 10 ms (100 Hz).

---

## 3. What Remains to Be Done (Verification Plan)

The entire codebase is written, integrated, and committed. The remaining work consists of runtime verification in a Linux/Ubuntu environment:

### Step 1: Toolchain Installation (Ubuntu 24.04 / WSL)
```bash
sudo apt update
sudo apt install -y build-essential g++-multilib make grub-pc-bin grub-common xorriso mtools qemu-system-x86 gdb
```

### Step 2: Build & Multiboot Verification
```bash
git pull origin main
make clean
make novaos.iso
```
*Verification criteria:* Ensure `grub-file --is-x86-multiboot build/novaos.bin` prints `Multiboot check: OK` with zero compiler/linker warnings.

### Step 3: QEMU Execution & Concurrency Test
```bash
make run
```
*Verification checklist:*
1. [ ] Boot banner appears with cyan/green status indicators.
2. [ ] Bottom row (row 24) displays spinning indicators for Tasks A, B, and C, proving simultaneous preemptive execution.
3. [ ] `novaos> ` prompt responds to keyboard input (letters, backspace, enter).
4. [ ] `tasks` command shows Task 0 (`RUNNING`), Tasks 1–3 (`READY`), and continuously incrementing context switch counts.
5. [ ] `start` command dynamically creates Task 4, and running `tasks` confirms Task 4 is enqueued and scheduled.
6. [ ] `uptime` accurately reports elapsed seconds and PIT tick counts.

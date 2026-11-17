# NovaOS

**A Minimal 32-bit x86 Operating System with Interactive Shell and Preemptive Multitasking**

NovaOS is a genuine educational operating system built from scratch following the incremental development methodology of the "Write your own Operating System" (WYOOS) tutorial series.

## Features

- **Real Preemptive Multitasking**: Timer-driven Round-Robin CPU context switching with independent task stacks
- **Interactive Shell**: Type commands to inspect tasks, create new tasks, and view system info
- **PS/2 Keyboard Driver**: Interrupt-driven keyboard input with scancode translation
- **PIT Timer**: 100 Hz Programmable Interval Timer driving the scheduler
- **Full Interrupt Infrastructure**: GDT, IDT, 8259 PIC remapping, assembly interrupt stubs

## Quick Start (Ubuntu 24.04)

```bash
# Install dependencies
sudo apt update
sudo apt install -y build-essential g++-multilib make grub-pc-bin grub-common \
     xorriso mtools qemu-system-x86 gdb

# Build
make clean
make novaos.iso

# Run in QEMU
make run

# Debug with GDB
make debug
```

## Shell Commands

| Command  | Description                              |
|----------|------------------------------------------|
| `help`   | List available commands                  |
| `info`   | Display system architecture information  |
| `tasks`  | Show all tasks with states and IDs       |
| `start`  | Create a new kernel task                 |
| `uptime` | Display uptime from real PIT tick count  |
| `clear`  | Clear the VGA screen                     |

## Architecture

```
GRUB Multiboot → loader.s → kernelMain()
                                ├── GDT (flat 4GB code+data segments)
                                ├── IDT + PIC (256 interrupt gates, 8259 remap)
                                ├── Keyboard Driver (IRQ1, scancode→ASCII)
                                ├── PIT Timer (IRQ0, 100Hz)
                                ├── Task Manager (Round-Robin scheduler)
                                │     ├── Boot/Shell task (slot 0)
                                │     ├── Task A (auto-created)
                                │     ├── Task B (auto-created)
                                │     └── Task C (auto-created)
                                └── Interactive Shell
```

## Project Structure

```
src/
  loader.s          - Multiboot header, stack, entry point
  interruptstubs.s  - Assembly interrupt stubs (register save/restore, iret)
  types.h           - Fixed-width integer types
  gdt.h/cpp         - Global Descriptor Table
  port.h/cpp        - Hardware I/O port classes
  interrupts.h/cpp  - IDT, PIC, interrupt dispatching
  keyboard.h/cpp    - PS/2 keyboard driver (IRQ1)
  timer.h/cpp       - PIT timer driver (IRQ0)
  multitasking.h/cpp - Task Control Block, Round-Robin scheduler
  console.h/cpp     - VGA text mode console with scrolling
  shell.h/cpp       - Interactive command shell
  kernel.cpp        - Kernel entry, initialization, main loop
linker.ld           - Kernel memory layout (1 MiB entry)
Makefile            - Build, ISO, run, debug targets
grub/grub.cfg       - GRUB bootloader menu
```

## How Context Switching Works

1. PIT fires IRQ0 → CPU pushes `eflags`, `cs`, `eip` automatically
2. `interruptstubs.s` pushes a dummy error code, then `pusha` (all GPRs), then segment registers
3. Calls `InterruptManager::HandleInterrupt(interrupt_number, esp)`
4. Timer handler calls `TaskManager::Schedule(esp)` which:
   - Saves current task's `esp` into its TCB
   - Selects next READY task (round-robin)
   - Returns the new task's saved `esp`
5. `interruptstubs.s` does `movl %eax, %esp` — now pointing at the new task's stack!
6. Pops segment registers, `popa`, skips error code, `iret` → new task resumes execution

## Limitations

- All tasks run in kernel mode (Ring 0) — no user-mode isolation
- No dynamic memory allocator (task stacks are statically allocated)
- No filesystem, networking, or disk I/O
- Maximum 16 concurrent tasks
- No task termination (tasks run forever once created)
- Single-core only

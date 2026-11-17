# NovaOS

A genuine educational 32-bit x86 operating system following the incremental architecture of the "Write your own Operating System" (WYOOS) series.

## Features & Highlights
- **Architecture:** 32-bit x86 Protected Mode (Multiboot 1 compliant).
- **Core Languages:** C++ for kernel modules, GNU Assembly for low-level CPU bootstrapping and interrupt stubs.
- **Hardware Abstraction:** Object-oriented Port I/O (`Port8Bit`, `Port16Bit`, `Port32Bit`), Global Descriptor Table (`GDT`), and Interrupt Descriptor Table (`IDT`).
- **Multitasking:** Preemptive Round-Robin task scheduling with real CPU context switching via timer interrupts.
- **Memory Management:** Dynamic kernel heap with chunk tracking and `new`/`delete` operators.
- **Diagnostics:** Interactive Shell and Live System Monitor.

---

## Current Status & Roadmap
For the comprehensive progress report, module breakdown, what has been built, and next implementation steps, see:
📄 **[PROGRESS_AND_TODO.md](PROGRESS_AND_TODO.md)**

---

## Setup & Compilation (Ubuntu / Debian)

```bash
# Install development toolchain & dependencies
sudo apt update
sudo apt install -y build-essential g++-multilib make grub-pc-bin grub-common xorriso mtools qemu-system-x86 gdb

# Build bootable ISO
make novaos.iso

# Execute in QEMU virtual machine
make run

# Clean build artifacts
make clean
```

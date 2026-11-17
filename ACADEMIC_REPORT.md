# NOVAOS — Academic Project Report & Technical Documentation

**Degree:** Bachelor of Technology (B.Tech) in Computer Science & Engineering  
**Subject:** Operating Systems / Low-Level Systems Programming  
**Project Title:** NovaOS — A Minimal 32-bit x86 Operating System with Interactive Shell and Preemptive Multitasking  
**Target Platform:** x86 Architecture (IA-32, 32-bit Protected Mode)  

---

## 1. Abstract
NovaOS is an educational, bare-metal 32-bit x86 operating system developed from first principles in C++ and GNU Assembly. Designed to bridge the gap between theoretical operating system concepts and real hardware execution, NovaOS implements a Multiboot-compliant boot protocol, segmentation via a Global Descriptor Table (GDT), an interrupt-driven architecture utilizing the 8259 Programmable Interrupt Controller (PIC) and Interrupt Descriptor Table (IDT), PS/2 keyboard decoding, an 8254 Programmable Interval Timer (PIT), and a preemptive Round-Robin task scheduler. Crucially, NovaOS does not simulate concurrency through cooperative function looping; instead, it executes genuine CPU register and stack context switches on hardware timer interrupts. An interactive command shell allows dynamic task creation, uptime querying, and real-time inspection of internal Task Control Blocks (TCBs).

---

## 2. Problem Statement
Most undergraduate curricula teach operating systems using abstract simulators, high-level user-space threading libraries, or monolithic production kernels such as Linux. These approaches present significant pedagogical limitations:
1. High-level threading models abstract away the hardware mechanics of stack pointer manipulation, interrupt frames, and assembly-level register preservation.
2. Monolithic production kernels (e.g., Linux, FreeBSD) exceed tens of millions of lines of code, making it virtually impossible for students to trace execution from bare-metal bootloader entry to task context switching.
3. Educational simulators often fake concurrency, concealing race conditions, PIC interrupt cascades, and low-level CPU state transitions.

The objective of NovaOS is to solve this by providing a compact, transparent, and fully functional operating system whose source code directly demonstrates each foundational hardware interface.

---

## 3. Project Objectives
1. **Bare-Metal Bootstrapping:** Initialize a 32-bit protected-mode kernel via GRUB and the Multiboot specification without relying on any host OS or standard C/C++ runtime library.
2. **Hardware Abstraction:** Implement clean, object-oriented abstractions for CPU descriptor tables (GDT, IDT) and port-mapped I/O (`in`/`out` assembly instructions).
3. **Interrupt Handling:** Correctly remap the legacy dual-8259 PIC vectors outside the CPU exception range and implement re-entrant assembly interrupt stubs.
4. **Preemptive Multitasking:** Construct isolated task stacks and implement a timer-driven Round-Robin scheduler executing genuine hardware context switches.
5. **Interactive Demonstration:** Provide an interrupt-safe command shell enabling manual task creation, real-time status inspection, and uptime tracking.

---

## 4. System Architecture

```
+-------------------------------------------------------------------------+
|                           User / Demo Space                             |
|    +-------------------+    +-------------------+    +----------------+ |
|    | Interactive Shell |    |  Demo Tasks (A-C) |    |  Dynamic Tasks | |
|    +---------+---------+    +---------+---------+    +--------+-------+ |
+--------------|------------------------|-----------------------|---------+
|              +-------------------+----+-----------------------+         |
|                                  |                                      |
|                                  v                                      |
|                       +--------------------+                            |
|                       |    TaskManager     |                            |
|                       | (Round-Robin TCBs) |                            |
|                       +---------+----------+                            |
|                                 |                                       |
|            +--------------------+---------------------+                 |
|            |                                          |                 |
|            v                                          v                 |
|   +-----------------+                        +------------------+       |
|   |  PIT Timer IRQ0 |                        | PS/2 Keyboard    |       |
|   |   (100 Hz Tick) |                        | (IRQ1 Scan Code) |       |
|   +--------+--------+                        +--------+---------+       |
|            |                                          |                 |
|            +--------------------+---------------------+                 |
|                                 |                                       |
|                                 v                                       |
|                    +--------------------------+                         |
|                    | InterruptManager & Stubs |                         |
|                    |  (8259 PIC + 256 IDT)    |                         |
|                    +------------+-------------+                         |
|                                 |                                       |
|                                 v                                       |
|                    +--------------------------+                         |
|                    | Global Descriptor Table  |                         |
|                    |     (Flat Ring 0 GDT)    |                         |
|                    +------------+-------------+                         |
|                                 |                                       |
|                                 v                                       |
|                    +--------------------------+                         |
|                    |  x86 Hardware / QEMU     |                         |
+--------------------+--------------------------+-------------------------+
```

---

## 5. Hardware and Software Requirements

### Hardware Requirements (Virtual or Physical)
- **Architecture:** x86-compatible CPU (Intel 80386 or higher, IA-32).
- **RAM:** Minimum 16 MiB (64 MiB configured in QEMU emulator).
- **Peripherals:** 8259 Programmable Interrupt Controller (PIC), 8254 Programmable Interval Timer (PIT), standard 80x25 color text-mode VGA adapter (`0xB8000`), PS/2 keyboard controller.

### Software Toolchain (Ubuntu 24.04 LTS / Debian)
- **Compiler:** `gcc` / `g++` (multilib support for `-m32`).
- **Assembler:** GNU `as` (`--32`).
- **Linker:** GNU `ld` (`-melf_i386`).
- **Bootloader:** GNU GRUB 2 (`grub-mkrescue`, `grub-pc-bin`).
- **ISO Tools:** `xorriso`, `mtools`.
- **Virtual Machine:** `qemu-system-i386`.
- **Debugger:** GNU Debugger (`gdb`).

---

## 6. Development Methodology
NovaOS follows an incremental, bottom-up systems engineering methodology modeled after the "Write your own Operating System" (WYOOS) educational approach:
1. **Stage 1–2 (Boot & Entry):** Multiboot verification, stack initialization, VGA buffer test.
2. **Stage 3–4 (CPU Segments & I/O):** Global Descriptor Table loading and hardware port abstraction classes.
3. **Stage 5 (Interrupts):** Dual-8259 PIC remapping, IDT vector initialization, assembly interrupt service routines (ISRs).
4. **Stage 6 (Input):** Interrupt-driven PS/2 keyboard driver with circular buffer.
5. **Stage 7 (Console & Shell):** Scrollable VGA terminal and text command parser.
6. **Stage 8 (Timer):** Programmable Interval Timer configuration generating 100 Hz periodic interrupts.
7. **Stage 9–10 (Multitasking):** Stack frame preservation, Task Control Block definition, and timer-driven Round-Robin context switching.

---

## 7. Boot Process
1. **BIOS Execution:** The BIOS initializes basic hardware, loads the Master Boot Record (MBR), and transfers control to GRUB.
2. **GRUB Bootloader:** GRUB searches for the Multiboot 1 signature (`0x1BADB002`) within the first 8 KiB of `novaos.bin`.
3. **Protected Mode Handover:** GRUB switches the CPU to 32-bit Protected Mode, disables interrupts (`cli`), sets `%eax = 0x2BADB002` (Multiboot magic), sets `%ebx = multiboot_info_ptr`, and jumps to the symbol `loader` defined in `src/loader.s`.
4. **Stack & Constructor Setup:** `loader.s` sets `%esp = kernel_stack + 2MB`, calls global C++ constructors via `callConstructors()`, and invokes `kernelMain()`.

---

## 8. Global Descriptor Table (GDT)
Although x86 protected mode requires segmentation, modern operating systems use a **Flat Memory Model** where segments cover the entire 4 GiB address space:
- **Null Descriptor (Selector `0x00`):** Required by x86 architecture; traps uninitialized segment register access.
- **Unused Descriptor (Selector `0x08`):** Preserved for alignment.
- **Kernel Code Segment (Selector `0x10`):** Base `0x00000000`, Limit `0xFFFFFFFF`, Access `0x9A` (Present, Ring 0, Executable, Readable).
- **Kernel Data Segment (Selector `0x18`):** Base `0x00000000`, Limit `0xFFFFFFFF`, Access `0x92` (Present, Ring 0, Writable).
Loaded into the GDTR using the `lgdt` instruction.

---

## 9. Interrupt Descriptor Table (IDT) & 8259 PIC
The IDT contains 256 8-byte gate descriptors.
### PIC Remapping
By default, the legacy IBM PC BIOS maps hardware IRQs 0–7 to interrupt vectors `0x08`–`0x0F`, which directly conflicts with x86 CPU exceptions (e.g., Double Fault, General Protection Fault). NovaOS remaps the 8259 PIC using Initialization Command Words (ICWs):
- **Master PIC (Ports `0x20`, `0x21`):** Remapped to base vector `0x20` (IRQs 0x00–0x07 → Vectors `0x20`–`0x27`).
- **Slave PIC (Ports `0xA0`, `0xA1`):** Remapped to base vector `0x28` (IRQs 0x08–0x0F → Vectors `0x28`–`0x2F`).
- **End-of-Interrupt (EOI):** Upon servicing an IRQ, the handler writes `0x20` to the Master PIC (and Slave PIC if IRQ $\ge 8$).

---

## 10. Keyboard Driver
The PS/2 keyboard driver (`KeyboardDriver`) inherits from `InterruptHandler` listening on vector `0x21` (IRQ1):
- **Port Communication:** Reads incoming scancodes from data port `0x60`.
- **Scancode Set 1:** Distinguishes between Make codes (key pressed, bit 7 clear) and Break codes (key released, bit 7 set).
- **State Machine:** Tracks toggle and modifier keys (Left/Right Shift, CapsLock).
- **Circular FIFO Buffer:** Decoded ASCII characters are buffered into a 128-byte queue (`getChar()` / `hasChar()`) for consumption by the shell without blocking the interrupt handler.

---

## 11. Programmable Interval Timer (PIT 8254)
The PIT operates from an oscillator input of $1.193182\text{ MHz}$.
- **Channel 0 (Port `0x40`):** Configured via Control Port `0x43` with command byte `0x36` (Square wave rate generator, 16-bit binary counter).
- **Frequency:** Set to 100 Hz ($10\text{ ms}$ quantum) using divisor:
  $$\text{Divisor} = \frac{1193182}{100} = 11932 \quad (\text{0x2E9C})$$
- **Functionality:** Increments global system tick count and invokes the preemptive scheduler on every tick.

---

## 12. Task Control Block (TCB) & Task Representation
Every process in NovaOS is represented by a `Task` structure:
```cpp
struct Task {
    uint8_t    stack[TASK_STACK_SIZE]; // 4 KiB private kernel stack
    CPUState*  cpuState;               // Saved register context
    int        id;                     // Unique task identifier
    TaskState  state;                  // READY, RUNNING, TERMINATED, UNUSED
};
```
Task stacks are statically allocated (up to 16 tasks), avoiding dynamic heap allocation complexity while guaranteeing memory isolation between task execution frames.

---

## 13. Preemptive Round-Robin Scheduling
The scheduler operates in `TaskManager::Schedule(uint32_t esp)`:
1. **State Preservation:** The currently executing task is marked `READY`, and its saved stack pointer `%esp` is stored in its TCB.
2. **Circular Scan:** The scheduler iterates through the task table starting from `(currentTask + 1) % MAX_TASKS` until it finds the next task in the `READY` state.
3. **State Activation:** The selected task is marked `RUNNING`, its context switch counter is incremented, and its saved `%esp` is returned.
4. **Boot Context Integration:** Task slot 0 is assigned to the boot/shell loop (`kernelMain`), ensuring the shell participates in Round-Robin scheduling alongside background tasks.

---

## 14. CPU Context-Switching Mechanism

Genuine context switching occurs through the cooperation of the timer ISR and `src/interruptstubs.s`:

### The Interrupt Stack Frame (`CPUState`)
When an interrupt fires while executing Task A, the CPU pushes:
```
[esp + 60]  EFLAGS
[esp + 56]  CS
[esp + 52]  EIP
```
The assembly stub (`int_bottom`) then pushes:
```
[esp + 48]  Error Code / Dummy (4 bytes)
[esp + 16]  EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX (pusha, 32 bytes)
[esp + 0]   GS, FS, ES, DS (16 bytes)
```

### The Stack Pointer Exchange
1. The stub pushes the current `%esp` as an argument to `InterruptManager::HandleInterrupt`.
2. The timer interrupt forwards this to `TaskManager::Schedule(esp)`.
3. The scheduler saves Task A's `%esp` and selects Task B, returning Task B's `%esp` in register `%eax`.
4. In `int_bottom`, the instruction `movl %eax, %esp` is executed!
5. Now pointing to Task B's stack, the stub executes `popl %ds`, `popl %es`, `popa`, `add $4, %esp`, and `iret`.
6. Register `%eip` is restored from Task B's stack, seamlessly resuming Task B's execution.

---

## 15. Interactive Shell
The shell runs as Task 0, processing non-blocking keyboard input and providing an interactive prompt (`novaos> `):
- `help`: Lists all available system commands.
- `info`: Displays operating system version, CPU architecture, protected mode status, timer, and scheduler diagnostics.
- `tasks`: Directly reads `TaskManager` data structures to report live task IDs, execution states (`RUNNING` / `READY`), active task count, and total context switches.
- `start`: Instantiates a new kernel task with an isolated 4 KiB stack, initializes its `CPUState` frame with `IF=1` (`0x202`), and enqueues it into the Round-Robin rotation.
- `uptime`: Calculates elapsed seconds and minutes from live PIT tick counts.
- `clear`: Flushes the 80x25 VGA buffer and resets the hardware cursor.

---

## 16. Testing Methodology & Verification
1. **Static Analysis & Build Verification:** Compiling with `-Wall -Wextra -ffreestanding -fno-pie` to detect memory alignment or syntax issues.
2. **Multiboot Compliance:** Using `grub-file --is-x86-multiboot` to verify the ELF binary header.
3. **QEMU Virtualization:** Booting `novaos.iso` under `qemu-system-i386 -cdrom novaos.iso -m 64M`.
4. **Concurrency Validation:** Observing simultaneous execution of 3 background demo tasks (spinning indicators on row 24 of the VGA display) while typing commands into the shell simultaneously.
5. **Dynamic Task Scaling:** Executing `start` multiple times and verifying through `tasks` that context switches increment continuously across all tasks.

---

## 17. Results
- **Boot Time:** Sub-second boot directly into 32-bit protected mode.
- **Multitasking:** Preemptive scheduling successfully alternates between 4+ tasks at 100 context switches per second without stack corruption or priority inversion.
- **Responsiveness:** Keyboard input remains responsive and buffered during heavy background task computation.
- **Code Size:** The entire operating system is implemented in approximately 1,200 lines of clean, structured C++ and Assembly.

---

## 18. Limitations
1. **Privilege Isolation:** All tasks run in Ring 0 (Kernel Mode). User-mode (Ring 3) isolation and Page Table protection are not implemented.
2. **Memory Allocation:** Task stacks are statically dimensioned; a dynamic heap allocator (`malloc` / paging) is omitted to prioritize scheduling reliability.
3. **Storage & Networking:** No permanent disk filesystem or network stack is present.
4. **Task Termination:** Tasks run indefinitely; explicit task killing and stack reclamation are not supported.

---

## 19. Future Scope
1. **Virtual Memory & Paging:** Implementing 4 KiB two-level page directories and page tables to isolate process address spaces.
2. **User Mode & Syscalls:** Transitioning to Ring 3 via TSS (Task State Segment) and software interrupt `int 0x80` or `sysenter`.
3. **File System Integration:** Adding an in-memory Ramdisk (Initrd) or simple FAT12 driver for executing standalone ELF binaries.
4. **Synchronization Primitives:** Implementing kernel semaphores, mutexes, and spinlocks for inter-task communication.

---

## 20. Conclusion
NovaOS demonstrates that building a genuine, preemptively multitasked operating system from bare metal is fully achievable within an educational framework. By following an incremental development methodology, the project demystifies the x86 protected mode environment, assembly interrupt stubs, hardware I/O communication, and stack-level context switching, providing an ideal foundation for academic evaluation and operating systems research.

---

## 21. Viva Questions and Answers

### Q1: What is the Multiboot specification and why is it used?
**Answer:** The Multiboot specification defines an open interface between bootloaders (like GRUB) and operating system kernels. By placing a valid Multiboot header (`0x1BADB002`) in the first 8 KiB of the binary, the kernel allows GRUB to load it directly into 32-bit protected mode, eliminating the need to write an error-prone, 16-bit real-mode custom boot sector.

### Q2: Why is the Global Descriptor Table (GDT) necessary if we use a flat memory model?
**Answer:** In x86 protected mode, the CPU hardware architecture strictly enforces that segment registers (`CS`, `DS`, `ES`, `SS`) contain valid selectors pointing to descriptors in the GDT. Even when using a flat memory model where all segments span $0\text{ to }4\text{ GiB}$, the GDT is required to define segment privilege levels (Ring 0 vs Ring 3) and execution permissions (Executable Code vs Writable Data).

### Q3: Why must the 8259 PIC be remapped during kernel initialization?
**Answer:** In IBM PC architecture, the BIOS maps Master PIC IRQs 0–7 to CPU interrupt vectors `0x08`–`0x0F`. However, Intel reserved CPU vectors `0x00`–`0x1F` for internal hardware exceptions (e.g., Vector `0x08` is Double Fault, Vector `0x0D` is General Protection Fault). Remapping the PIC moves hardware interrupts to vectors `0x20`–`0x2F`, preventing hardware IRQs from masquerading as catastrophic CPU exceptions.

### Q4: How does NovaOS achieve preemptive multitasking rather than cooperative multitasking?
**Answer:** In cooperative multitasking, a task must voluntarily call `yield()` to surrender the CPU. In NovaOS, the 8254 PIT hardware timer periodically generates IRQ0 interrupts (100 times per second). Because interrupts are enabled (`sti`), the CPU hardware forcibly interrupts whatever task is currently executing, pushes its context, and invokes the scheduler, preemptively transferring execution to another task.

### Q5: Explain how a context switch actually changes which code is running.
**Answer:** The interrupt stub pushes the registers of Task A onto Task A's stack. The scheduler then selects Task B and changes the CPU stack pointer register (`%esp`) to point to Task B's stack. When the stub executes `popa` and `iret`, the CPU pops registers, segment selectors, and the instruction pointer (`%eip`) from Task B's stack, immediately resuming execution at Task B's saved code location.

### Q6: What role does the `eflags` register play during initial task creation?
**Answer:** When creating a new task, its initial stack frame is synthesized with `eflags = 0x202`. Bit 9 is the Interrupt Flag (IF). Setting IF=1 ensures that when the task is first started via `iret`, hardware interrupts remain enabled so the timer can continue preempting it.

### Q7: What is the difference between `inb`/`outb` and memory operations?
**Answer:** x86 features a separate I/O port address space accessed exclusively through special `in` and `out` assembly instructions. Unlike memory-mapped I/O which maps hardware registers into standard RAM addresses, port-mapped I/O communicates with external peripheral controllers (such as the PIC at port `0x20`, PIT at port `0x40`, and keyboard at port `0x60`) using dedicated CPU bus cycles.

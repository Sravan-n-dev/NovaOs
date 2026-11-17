#include "types.h"
#include "gdt.h"
#include "interrupts.h"
#include "keyboard.h"
#include "timer.h"
#include "multitasking.h"
#include "console.h"
#include "shell.h"

// C++ global constructor support
typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors() {
    for (constructor* c = &start_ctors; c != &end_ctors; c++) (*c)();
}

// ==================== Automatic Kernel Tasks ====================
// These demonstrate real preemptive multitasking.
// Each task runs in an infinite loop on its own stack.
// The timer interrupt preempts them and switches to the next task.

// Task A: incrementing counter, writes to a known VGA cell
static volatile uint32_t taskA_counter = 0;
void taskA() {
    while (1) {
        taskA_counter++;
        // Show counter activity on VGA row 24, col 0 (bottom-left status)
        volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
        // Display 'A' followed by a spinner based on counter
        const char spinner[] = "|/-\\";
        vga[24 * 80 + 0] = 0x0E00 | 'A';
        vga[24 * 80 + 1] = 0x0E00 | ':';
        vga[24 * 80 + 2] = 0x0A00 | spinner[(taskA_counter >> 14) & 3];

        for (volatile int d = 0; d < 30000; d++);
    }
}

// Task B: separate counter, writes to different VGA position
static volatile uint32_t taskB_counter = 0;
void taskB() {
    while (1) {
        taskB_counter++;
        volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
        const char spinner[] = "|/-\\";
        vga[24 * 80 + 5] = 0x0D00 | 'B';
        vga[24 * 80 + 6] = 0x0D00 | ':';
        vga[24 * 80 + 7] = 0x0A00 | spinner[(taskB_counter >> 14) & 3];

        for (volatile int d = 0; d < 30000; d++);
    }
}

// Task C: third independent counter
static volatile uint32_t taskC_counter = 0;
void taskC() {
    while (1) {
        taskC_counter++;
        volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
        const char spinner[] = "|/-\\";
        vga[24 * 80 + 10] = 0x0B00 | 'C';
        vga[24 * 80 + 11] = 0x0B00 | ':';
        vga[24 * 80 + 12] = 0x0A00 | spinner[(taskC_counter >> 14) & 3];

        for (volatile int d = 0; d < 30000; d++);
    }
}

// ==================== Kernel Entry Point ====================

extern "C" void kernelMain(void* multibootInfo, uint32_t magic) {
    (void)multibootInfo;

    // Use the console for all output from here
    console.clear();

    console.setColor(COLOR_WHITE, COLOR_BLUE);
    console.printLine("  NovaOS 0.1 - 32-bit x86 Educational Operating System                          ");
    console.setColor(COLOR_WHITE);
    console.printLine("");

    // Verify Multiboot
    if (magic == 0x2BADB002) {
        console.setColor(COLOR_LGREEN);
        console.printLine("[OK] Multiboot magic verified.");
    } else {
        console.setColor(COLOR_LRED);
        console.printLine("[FAIL] Invalid Multiboot magic!");
        while (1) asm volatile("hlt");
    }

    // 1. Initialize GDT
    GlobalDescriptorTable gdt;
    console.setColor(COLOR_LGREEN);
    console.printLine("[OK] GDT initialized.");

    // 2. Initialize IDT + PIC
    InterruptManager interrupts(0x20, &gdt);
    console.printLine("[OK] IDT initialized, PIC remapped.");

    // 3. Keyboard driver (registers itself as IRQ1 handler)
    KeyboardDriver keyboard(&interrupts);
    console.printLine("[OK] Keyboard initialized.");

    // 4. Timer (registers itself as IRQ0 handler)
    Timer timer(&interrupts);
    console.printLine("[OK] PIT initialized (100 Hz).");

    // 5. Task Manager
    TaskManager taskManager;

    // Create three automatic kernel tasks
    int idA = taskManager.CreateTask(taskA);
    int idB = taskManager.CreateTask(taskB);
    int idC = taskManager.CreateTask(taskC);

    console.setColor(COLOR_LCYAN);
    console.print("[OK] Created tasks: ");
    console.printInt(idA); console.print(", ");
    console.printInt(idB); console.print(", ");
    console.printInt(idC); console.printLine("");

    // 6. Connect timer to scheduler
    timer.SetTaskManager(&taskManager);
    console.setColor(COLOR_LGREEN);
    console.printLine("[OK] Scheduler initialized (Round Robin).");
    console.setColor(COLOR_WHITE);

    console.printLine("");
    console.printLine("Type 'help' for available commands.");
    console.printLine("");

    // 7. Enable interrupts — this starts everything!
    // Timer will fire, scheduler will preempt, tasks will run.
    interrupts.Activate();

    // 8. Shell (runs in the boot/idle context — also scheduled)
    Shell shell(&taskManager);
    shell.prompt();

    // Main kernel loop: process keyboard input between interrupts
    while (1) {
        shell.update();
        asm volatile("hlt");  // sleep until next interrupt
    }
}

#include "types.h"
#include "gdt.h"
#include "interrupts.h"

typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors() {
    for (constructor* c = &start_ctors; c != &end_ctors; c++) (*c)();
}

// Minimal VGA text output: 80x25 cells at 0xB8000, 2 bytes each (char, color)
static void print(const char* s, int row, uint8_t color = 0x0B) {
    volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
    for (int i = 0; s[i]; i++) {
        vga[row * 80 + i] = ((uint16_t)color << 8) | (uint8_t)s[i];
    }
}

extern "C" void kernelMain(void* multibootInfo, uint32_t magic) {
    (void)multibootInfo;

    // Clear screen with grey on black
    volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
    for (int i = 0; i < 80 * 25; i++) {
        vga[i] = 0x0720;
    }

    print("==================================================", 0, 0x0F);
    print("           NovaOS Kernel v0.5 (WYOOS)            ", 1, 0x0E);
    print("==================================================", 2, 0x0F);

    if (magic == 0x2BADB002) {
        print("[OK] Multiboot Header Verified (0x2BADB002)", 4, 0x0A);
    } else {
        print("[FAIL] Invalid Multiboot Magic", 4, 0x0C);
    }

    // Step 1: Initialize Global Descriptor Table (GDT)
    print("[..] Initializing Global Descriptor Table...", 5, 0x07);
    GlobalDescriptorTable gdt;
    print("[OK] GDT initialized (Code/Data Segments Loaded)", 5, 0x0A);

    // Step 2: Initialize Interrupt Descriptor Table (IDT) & PIC Remap (0x20..0x2F)
    print("[..] Initializing Interrupts and 8259 PIC...", 6, 0x07);
    InterruptManager interrupts(0x20, &gdt);
    print("[OK] IDT loaded & 8259 PIC remapped to 0x20-0x2F", 6, 0x0A);

    // Step 3: Activate hardware interrupts (sti)
    interrupts.Activate();
    print("[OK] Hardware interrupts enabled (STI active)", 7, 0x0A);

    print("NovaOS is running. Halting CPU until next interrupt...", 9, 0x0B);

    while (1) {
        asm volatile("hlt");
    }
}

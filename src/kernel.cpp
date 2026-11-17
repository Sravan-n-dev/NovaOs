#include "types.h"

typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors() {
    for (constructor* c = &start_ctors; c != &end_ctors; c++) (*c)();
}

// Minimal VGA text output: 80x25 cells at 0xB8000, 2 bytes each (char, color)
static void print(const char* s, int row) {
    volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
    for (int i = 0; s[i]; i++) vga[row * 80 + i] = 0x0B00 | s[i]; // cyan on black
}

extern "C" void kernelMain(void* multibootInfo, uint32_t magic) {
    (void)multibootInfo;
    for (int i = 0; i < 80 * 25; i++) ((volatile uint16_t*)0xB8000)[i] = 0x0720; // clear
    print("NovaOS booted.", 0);
    if (magic == 0x2BADB002) print("Multiboot magic OK", 1);
    else                     print("Multiboot magic BAD", 1);
    while (1) asm volatile("hlt");
}

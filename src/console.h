#ifndef CONSOLE_H
#define CONSOLE_H

#include "types.h"

// 80x25 VGA text mode: Rows 0..23 for interactive shell terminal,
// Row 24 reserved as a dedicated hardware task monitor status bar.
#define VGA_WIDTH  80
#define VGA_HEIGHT 24
#define VGA_TOTAL_HEIGHT 25
#define VGA_MEMORY ((volatile uint16_t*)0xB8000)

// Colors
#define COLOR_BLACK     0x0
#define COLOR_BLUE      0x1
#define COLOR_GREEN     0x2
#define COLOR_CYAN      0x3
#define COLOR_RED       0x4
#define COLOR_MAGENTA   0x5
#define COLOR_BROWN     0x6
#define COLOR_LGRAY     0x7
#define COLOR_DGRAY     0x8
#define COLOR_LBLUE     0x9
#define COLOR_LGREEN    0xA
#define COLOR_LCYAN     0xB
#define COLOR_LRED      0xC
#define COLOR_LMAGENTA  0xD
#define COLOR_YELLOW    0xE
#define COLOR_WHITE     0xF

class Console {
    int cursorX;
    int cursorY;
    uint8_t color;  // foreground in lower nibble, background in upper nibble

    void scroll();
    void updateHardwareCursor();

public:
    Console();

    void clear();
    void setColor(uint8_t fg, uint8_t bg = COLOR_BLACK);

    void putChar(char c);
    void print(const char* str);
    void printLine(const char* str);
    void printInt(int value);
    void printHex(uint32_t value);

    int getCursorX();
    int getCursorY();
};

// Global console instance (extern)
extern Console console;

#endif

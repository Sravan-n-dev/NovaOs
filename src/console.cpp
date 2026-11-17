#include "console.h"
#include "port.h"

Console console;  // global instance

Console::Console()
    : cursorX(0), cursorY(0), color(0x07)  // light gray on black
{
}

void Console::scroll() {
    // Scroll terminal region (rows 0 through VGA_HEIGHT - 1)
    for (int y = 0; y < VGA_HEIGHT - 1; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            VGA_MEMORY[y * VGA_WIDTH + x] = VGA_MEMORY[(y + 1) * VGA_WIDTH + x];
        }
    }
    // Clear the bottom line of the scroll region
    for (int x = 0; x < VGA_WIDTH; x++) {
        VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = ((uint16_t)color << 8) | ' ';
    }
}

void Console::updateHardwareCursor() {
    uint16_t pos = cursorY * VGA_WIDTH + cursorX;
    Port8Bit cmdPort(0x3D4);
    Port8Bit dataPort(0x3D5);
    cmdPort.Write(0x0F);
    dataPort.Write((uint8_t)(pos & 0xFF));
    cmdPort.Write(0x0E);
    dataPort.Write((uint8_t)((pos >> 8) & 0xFF));
}

void Console::clear() {
    // Clear entire VGA frame buffer
    for (int i = 0; i < VGA_WIDTH * VGA_TOTAL_HEIGHT; i++) {
        VGA_MEMORY[i] = ((uint16_t)color << 8) | ' ';
    }
    cursorX = 0;
    cursorY = 0;
    updateHardwareCursor();
}

void Console::setColor(uint8_t fg, uint8_t bg) {
    color = (bg << 4) | (fg & 0x0F);
}

void Console::putChar(char c) {
    if (c == '\n') {
        cursorX = 0;
        cursorY++;
    } else if (c == '\b') {
        if (cursorX > 0) {
            cursorX--;
            VGA_MEMORY[cursorY * VGA_WIDTH + cursorX] = ((uint16_t)color << 8) | ' ';
        }
    } else if (c == '\t') {
        cursorX = (cursorX + 4) & ~3;  // align to next 4-col boundary
    } else if (c >= ' ') {
        VGA_MEMORY[cursorY * VGA_WIDTH + cursorX] = ((uint16_t)color << 8) | (uint8_t)c;
        cursorX++;
    }

    if (cursorX >= VGA_WIDTH) {
        cursorX = 0;
        cursorY++;
    }

    while (cursorY >= VGA_HEIGHT) {
        scroll();
        cursorY--;
    }

    updateHardwareCursor();
}

void Console::print(const char* str) {
    for (int i = 0; str[i]; i++) {
        putChar(str[i]);
    }
}

void Console::printLine(const char* str) {
    print(str);
    putChar('\n');
}

void Console::printInt(int value) {
    if (value < 0) {
        putChar('-');
        value = -value;
    }
    if (value == 0) {
        putChar('0');
        return;
    }

    char buf[12];
    int pos = 0;
    while (value > 0) {
        buf[pos++] = '0' + (value % 10);
        value /= 10;
    }
    // Print in reverse
    for (int i = pos - 1; i >= 0; i--) {
        putChar(buf[i]);
    }
}

void Console::printHex(uint32_t value) {
    const char* hex = "0123456789ABCDEF";
    print("0x");
    for (int i = 28; i >= 0; i -= 4) {
        putChar(hex[(value >> i) & 0xF]);
    }
}

int Console::getCursorX() { return cursorX; }
int Console::getCursorY() { return cursorY; }

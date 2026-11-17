#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "types.h"
#include "port.h"
#include "interrupts.h"

// Circular keyboard buffer
#define KEYBUF_SIZE 128

class KeyboardDriver : public InterruptHandler {
    Port8Bit dataPort;
    Port8Bit commandPort;

    // Circular input buffer
    static char buffer[KEYBUF_SIZE];
    static volatile int bufHead;
    static volatile int bufTail;

    bool shiftPressed;
    bool capsLock;

    // Scancode set 1 -> ASCII translation
    char scancodeToAscii(uint8_t scancode);

public:
    KeyboardDriver(InterruptManager* manager);
    ~KeyboardDriver();

    virtual uint32_t HandleInterrupt(uint32_t esp);

    // Called from shell to read a character (non-blocking, returns 0 if empty)
    static char getChar();
    static bool hasChar();
};

#endif

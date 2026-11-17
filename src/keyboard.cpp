#include "keyboard.h"

// Static member definitions
char KeyboardDriver::buffer[KEYBUF_SIZE];
volatile int KeyboardDriver::bufHead = 0;
volatile int KeyboardDriver::bufTail = 0;

KeyboardDriver::KeyboardDriver(InterruptManager* manager)
    : InterruptHandler(manager, 0x21),  // IRQ1 = interrupt 0x21
      dataPort(0x60),
      commandPort(0x64),
      shiftPressed(false),
      capsLock(false)
{
    // Wait for keyboard controller to be ready, then enable
    while (commandPort.Read() & 0x1)
        dataPort.Read();  // flush output buffer
    commandPort.Write(0xAE);  // enable keyboard interface
    while (commandPort.Read() & 0x1)
        dataPort.Read();
    commandPort.Write(0x20);  // read controller config
    while (!(commandPort.Read() & 0x1))
        ;
    uint8_t status = (dataPort.Read() | 1) & ~0x10;  // enable IRQ, enable clock
    commandPort.Write(0x60);  // write controller config
    dataPort.Write(status);
    dataPort.Write(0xF4);     // enable scanning
}

KeyboardDriver::~KeyboardDriver() {}

// Scancode Set 1 -> ASCII mapping (US QWERTY)
static const char scancode_normal[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,  'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,  '\\','z','x','c','v','b','n','m',',','.','/', 0,
    '*', 0, ' '
};

static const char scancode_shift[128] = {
    0,  27, '!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,  'A','S','D','F','G','H','J','K','L',':','"','~',
    0,  '|','Z','X','C','V','B','N','M','<','>','?', 0,
    '*', 0, ' '
};

char KeyboardDriver::scancodeToAscii(uint8_t scancode) {
    if (scancode >= 128) return 0;

    bool useShift = shiftPressed;
    // CapsLock inverts shift for letters only
    char normal = scancode_normal[scancode];
    if (capsLock && normal >= 'a' && normal <= 'z')
        useShift = !useShift;

    if (useShift)
        return scancode_shift[scancode];
    else
        return scancode_normal[scancode];
}

uint32_t KeyboardDriver::HandleInterrupt(uint32_t esp) {
    uint8_t scancode = dataPort.Read();

    // Key release (bit 7 set)
    if (scancode & 0x80) {
        uint8_t released = scancode & 0x7F;
        if (released == 0x2A || released == 0x36) // Left/Right Shift released
            shiftPressed = false;
        return esp;
    }

    // Key press
    switch (scancode) {
        case 0x2A: // Left Shift
        case 0x36: // Right Shift
            shiftPressed = true;
            return esp;
        case 0x3A: // CapsLock toggle
            capsLock = !capsLock;
            return esp;
    }

    char c = scancodeToAscii(scancode);
    if (c != 0) {
        // Add to circular buffer
        int nextHead = (bufHead + 1) % KEYBUF_SIZE;
        if (nextHead != bufTail) {  // buffer not full
            buffer[bufHead] = c;
            bufHead = nextHead;
        }
    }

    return esp;
}

char KeyboardDriver::getChar() {
    if (bufHead == bufTail) return 0;  // empty
    char c = buffer[bufTail];
    bufTail = (bufTail + 1) % KEYBUF_SIZE;
    return c;
}

bool KeyboardDriver::hasChar() {
    return bufHead != bufTail;
}

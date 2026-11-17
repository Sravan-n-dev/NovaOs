#include "port.h"

Port::Port(uint16_t portnumber) : portnumber(portnumber) {}
Port::~Port() {}

// --- Port8Bit ---
Port8Bit::Port8Bit(uint16_t portnumber) : Port(portnumber) {}
Port8Bit::~Port8Bit() {}

uint8_t Port8Bit::Read() {
    return Read8(portnumber);
}

void Port8Bit::Write(uint8_t data) {
    Write8(portnumber, data);
}

// --- Port8BitSlow ---
Port8BitSlow::Port8BitSlow(uint16_t portnumber) : Port8Bit(portnumber) {}
Port8BitSlow::~Port8BitSlow() {}

void Port8BitSlow::Write(uint8_t data) {
    Write8(portnumber, data);
    // small delay by writing to unused port 0x80
    asm volatile("outb %%al, $0x80" : : "a"(data));
}

// --- Port16Bit ---
Port16Bit::Port16Bit(uint16_t portnumber) : Port(portnumber) {}
Port16Bit::~Port16Bit() {}

uint16_t Port16Bit::Read() {
    uint16_t result;
    asm volatile("inw %1, %0" : "=a"(result) : "Nd"(portnumber));
    return result;
}

void Port16Bit::Write(uint16_t data) {
    asm volatile("outw %0, %1" : : "a"(data), "Nd"(portnumber));
}

// --- Port32Bit ---
Port32Bit::Port32Bit(uint16_t portnumber) : Port(portnumber) {}
Port32Bit::~Port32Bit() {}

uint32_t Port32Bit::Read() {
    uint32_t result;
    asm volatile("inl %1, %0" : "=a"(result) : "Nd"(portnumber));
    return result;
}

void Port32Bit::Write(uint32_t data) {
    asm volatile("outl %0, %1" : : "a"(data), "Nd"(portnumber));
}

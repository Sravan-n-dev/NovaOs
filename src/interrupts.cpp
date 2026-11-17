#include "interrupts.h"

// Static member definitions
InterruptManager::GateDescriptor InterruptManager::interruptDescriptorTable[256];
InterruptManager* InterruptManager::ActiveInterruptManager = 0;

// Base InterruptHandler implementation
InterruptHandler::InterruptHandler(InterruptManager* interruptManager, uint8_t interruptNumber)
    : interruptNumber(interruptNumber), interruptManager(interruptManager)
{
    if (interruptManager != 0) {
        interruptManager->handlers[interruptNumber] = this;
    }
}

InterruptHandler::~InterruptHandler() {
    if (interruptManager != 0 && interruptManager->handlers[interruptNumber] == this) {
        interruptManager->handlers[interruptNumber] = 0;
    }
}

uint32_t InterruptHandler::HandleInterrupt(uint32_t esp) {
    return esp;
}

// Set individual IDT gate entry
void InterruptManager::SetInterruptDescriptorTableEntry(uint8_t interrupt,
    uint16_t codeSegmentSelectorOffset, void (*handler)(),
    uint8_t DescriptorPrivilegeLevel, uint8_t DescriptorType)
{
    const uint8_t IDT_DESC_PRESENT = 0x80;
    interruptDescriptorTable[interrupt].handlerAddressLowBits = ((uint32_t)handler) & 0xFFFF;
    interruptDescriptorTable[interrupt].handlerAddressHighBits = (((uint32_t)handler) >> 16) & 0xFFFF;
    interruptDescriptorTable[interrupt].gdt_codeSegmentSelector = codeSegmentSelectorOffset;
    interruptDescriptorTable[interrupt].access = IDT_DESC_PRESENT | ((DescriptorPrivilegeLevel & 3) << 5) | DescriptorType;
    interruptDescriptorTable[interrupt].reserved = 0;
}

InterruptManager::InterruptManager(uint16_t hardwareInterruptOffset, GlobalDescriptorTable* globalDescriptorTable)
    : hardwareInterruptOffset(hardwareInterruptOffset),
      programmableInterruptControllerMasterCommandPort(0x20),
      programmableInterruptControllerMasterDataPort(0x21),
      programmableInterruptControllerSlaveCommandPort(0xA0),
      programmableInterruptControllerSlaveDataPort(0xA1)
{
    uint16_t CodeSegment = globalDescriptorTable->CodeSegmentSelector();
    const uint8_t IDT_INTERRUPT_GATE = 0xE;

    for (int i = 0; i < 256; i++) {
        handlers[i] = 0;
        SetInterruptDescriptorTableEntry(i, CodeSegment, &InterruptIgnore, 0, IDT_INTERRUPT_GATE);
    }

    // Connect CPU Exceptions (0x00 - 0x13)
    SetInterruptDescriptorTableEntry(0x00, CodeSegment, &HandleException0x00, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(0x01, CodeSegment, &HandleException0x01, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(0x02, CodeSegment, &HandleException0x02, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(0x03, CodeSegment, &HandleException0x03, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(0x04, CodeSegment, &HandleException0x04, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(0x05, CodeSegment, &HandleException0x05, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(0x06, CodeSegment, &HandleException0x06, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(0x07, CodeSegment, &HandleException0x07, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(0x08, CodeSegment, &HandleException0x08, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(0x09, CodeSegment, &HandleException0x09, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(0x0A, CodeSegment, &HandleException0x0A, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(0x0B, CodeSegment, &HandleException0x0B, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(0x0C, CodeSegment, &HandleException0x0C, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(0x0D, CodeSegment, &HandleException0x0D, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(0x0E, CodeSegment, &HandleException0x0E, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(0x0F, CodeSegment, &HandleException0x0F, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(0x10, CodeSegment, &HandleException0x10, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(0x11, CodeSegment, &HandleException0x11, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(0x12, CodeSegment, &HandleException0x12, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(0x13, CodeSegment, &HandleException0x13, 0, IDT_INTERRUPT_GATE);

    // Connect Hardware IRQs (Master: 0x20..0x27, Slave: 0x28..0x2F)
    SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x00, CodeSegment, &HandleInterruptRequest0x00, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x01, CodeSegment, &HandleInterruptRequest0x01, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x02, CodeSegment, &HandleInterruptRequest0x02, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x03, CodeSegment, &HandleInterruptRequest0x03, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x04, CodeSegment, &HandleInterruptRequest0x04, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x05, CodeSegment, &HandleInterruptRequest0x05, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x06, CodeSegment, &HandleInterruptRequest0x06, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x07, CodeSegment, &HandleInterruptRequest0x07, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x08, CodeSegment, &HandleInterruptRequest0x08, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x09, CodeSegment, &HandleInterruptRequest0x09, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x0A, CodeSegment, &HandleInterruptRequest0x0A, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x0B, CodeSegment, &HandleInterruptRequest0x0B, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x0C, CodeSegment, &HandleInterruptRequest0x0C, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x0D, CodeSegment, &HandleInterruptRequest0x0D, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x0E, CodeSegment, &HandleInterruptRequest0x0E, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x0F, CodeSegment, &HandleInterruptRequest0x0F, 0, IDT_INTERRUPT_GATE);

    // Remap the 8259 PIC (Programmable Interrupt Controller)
    // ICW1: Start initialization in cascade mode
    programmableInterruptControllerMasterCommandPort.Write(0x11);
    programmableInterruptControllerSlaveCommandPort.Write(0x11);

    // ICW2: Vector offsets
    programmableInterruptControllerMasterDataPort.Write(hardwareInterruptOffset);
    programmableInterruptControllerSlaveDataPort.Write(hardwareInterruptOffset + 8);

    // ICW3: Cascading connection (Slave at IRQ2)
    programmableInterruptControllerMasterDataPort.Write(0x04);
    programmableInterruptControllerSlaveDataPort.Write(0x02);

    // ICW4: 8086 mode
    programmableInterruptControllerMasterDataPort.Write(0x01);
    programmableInterruptControllerSlaveDataPort.Write(0x01);

    // OCW1: Unmask all interrupts
    programmableInterruptControllerMasterDataPort.Write(0x00);
    programmableInterruptControllerSlaveDataPort.Write(0x00);

    // Load IDT into CPU using LIDT
    struct {
        uint16_t limit;
        uint32_t base;
    } __attribute__((packed)) idt_pointer;

    idt_pointer.limit = sizeof(interruptDescriptorTable) - 1;
    idt_pointer.base = (uint32_t)interruptDescriptorTable;
    asm volatile("lidt %0" : : "m"(idt_pointer));
}

InterruptManager::~InterruptManager() {
    Deactivate();
}

uint16_t InterruptManager::HardwareInterruptOffset() {
    return hardwareInterruptOffset;
}

void InterruptManager::Activate() {
    if (ActiveInterruptManager != 0) {
        ActiveInterruptManager->Deactivate();
    }
    ActiveInterruptManager = this;
    asm volatile("sti");
}

void InterruptManager::Deactivate() {
    if (ActiveInterruptManager == this) {
        ActiveInterruptManager = 0;
        asm volatile("cli");
    }
}

uint32_t InterruptManager::HandleInterrupt(uint8_t interrupt, uint32_t esp) {
    if (ActiveInterruptManager != 0) {
        return ActiveInterruptManager->DoHandleInterrupt(interrupt, esp);
    }
    return esp;
}

uint32_t InterruptManager::DoHandleInterrupt(uint8_t interrupt, uint32_t esp) {
    // Dispatch to registered handler if one exists
    if (handlers[interrupt] != 0) {
        esp = handlers[interrupt]->HandleInterrupt(esp);
    }

    // Send EOI (End of Interrupt) for hardware IRQs
    if (hardwareInterruptOffset <= interrupt && interrupt < hardwareInterruptOffset + 16) {
        // If from slave PIC, send EOI to slave PIC
        if (hardwareInterruptOffset + 8 <= interrupt) {
            programmableInterruptControllerSlaveCommandPort.Write(0x20);
        }
        // Always send EOI to master PIC
        programmableInterruptControllerMasterCommandPort.Write(0x20);
    }

    return esp;
}

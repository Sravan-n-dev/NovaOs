# Assembly interrupt entry stubs for NovaOS

.set IRQ_BASE, 0x20

.section .text
.extern _ZN16InterruptManager15HandleInterruptEhj
.global _ZN16InterruptManager15InterruptIgnoreEv

_ZN16InterruptManager15InterruptIgnoreEv:
    iret

.data
    interruptNumber: .byte 0

.section .text

.macro HandleException num
.global _ZN16InterruptManager16HandleException\num\()Ev
_ZN16InterruptManager16HandleException\num\()Ev:
    movb $\num, (interruptNumber)
    pushl $0
    jmp int_bottom
.endm

.macro HandleExceptionWithError num
.global _ZN16InterruptManager16HandleException\num\()Ev
_ZN16InterruptManager16HandleException\num\()Ev:
    movb $\num, (interruptNumber)
    jmp int_bottom
.endm

.macro HandleInterruptRequest num
.global _ZN16InterruptManager23HandleInterruptRequest\num\()Ev
_ZN16InterruptManager23HandleInterruptRequest\num\()Ev:
    movb $\num + IRQ_BASE, (interruptNumber)
    pushl $0
    jmp int_bottom
.endm

# CPU Exceptions
HandleException 0x00
HandleException 0x01
HandleException 0x02
HandleException 0x03
HandleException 0x04
HandleException 0x05
HandleException 0x06
HandleException 0x07
HandleExceptionWithError 0x08
HandleException 0x09
HandleExceptionWithError 0x0A
HandleExceptionWithError 0x0B
HandleExceptionWithError 0x0C
HandleExceptionWithError 0x0D
HandleExceptionWithError 0x0E
HandleException 0x0F
HandleException 0x10
HandleExceptionWithError 0x11
HandleException 0x12
HandleException 0x13

# Hardware IRQs (0x00 - 0x0F)
HandleInterruptRequest 0x00
HandleInterruptRequest 0x01
HandleInterruptRequest 0x02
HandleInterruptRequest 0x03
HandleInterruptRequest 0x04
HandleInterruptRequest 0x05
HandleInterruptRequest 0x06
HandleInterruptRequest 0x07
HandleInterruptRequest 0x08
HandleInterruptRequest 0x09
HandleInterruptRequest 0x0A
HandleInterruptRequest 0x0B
HandleInterruptRequest 0x0C
HandleInterruptRequest 0x0D
HandleInterruptRequest 0x0E
HandleInterruptRequest 0x0F

int_bottom:
    # Save all general registers
    pusha
    # Save data segment registers
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs

    # Call C++ InterruptManager::HandleInterrupt(uint8_t interrupt, uint32_t esp)
    pushl %esp
    pushl (interruptNumber)
    call _ZN16InterruptManager15HandleInterruptEhj
    # Returned %eax is the (potentially updated) stack pointer for multitasking!
    movl %eax, %esp

    # Restore segment registers
    popl %gs
    popl %fs
    popl %es
    popl %ds
    # Restore general registers
    popa
    # Skip error code and interrupt number
    add $4, %esp
    iret

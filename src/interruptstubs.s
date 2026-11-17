# Assembly interrupt entry stubs for NovaOS
# Matches C++ mangled names from InterruptManager with -fno-leading-underscore

.set IRQ_BASE, 0x20

.section .text
.extern _ZN16InterruptManager15HandleInterruptEhj
.global _ZN16InterruptManager15InterruptIgnoreEv

_ZN16InterruptManager15InterruptIgnoreEv:
    iret

.data
    interruptNumber: .byte 0

.section .text

# HandleException0x## -> method name is 19 chars (HandleException + 0x##)
# Mangled: _ZN16InterruptManager19HandleException0x##Ev
.macro HandleException num
.global _ZN16InterruptManager19HandleException\num\()Ev
_ZN16InterruptManager19HandleException\num\()Ev:
    movb $\num, (interruptNumber)
    pushl $0
    jmp int_bottom
.endm

.macro HandleExceptionWithError num
.global _ZN16InterruptManager19HandleException\num\()Ev
_ZN16InterruptManager19HandleException\num\()Ev:
    movb $\num, (interruptNumber)
    jmp int_bottom
.endm

# HandleInterruptRequest0x## -> method name is 26 chars
# Mangled: _ZN16InterruptManager26HandleInterruptRequest0x##Ev
.macro HandleInterruptRequest num
.global _ZN16InterruptManager26HandleInterruptRequest\num\()Ev
_ZN16InterruptManager26HandleInterruptRequest\num\()Ev:
    movb $\num + IRQ_BASE, (interruptNumber)
    pushl $0
    jmp int_bottom
.endm

# CPU Exceptions 0x00 - 0x13
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

# Hardware IRQs 0x00 - 0x0F
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
    # Save all general-purpose registers (EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX)
    pusha
    # Save segment registers
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs

    # Call C++ handler: InterruptManager::HandleInterrupt(uint8_t interrupt, uint32_t esp)
    # arg2 (pushed first): esp (pointer to saved context at gs)
    # arg1 (pushed second): interrupt number (zero-extended to 32 bits)
    pushl %esp
    movzbl (interruptNumber), %eax
    pushl %eax
    call _ZN16InterruptManager15HandleInterruptEhj

    # Returned %eax is the (potentially new) stack pointer for multitasking
    # In TaskManager::Schedule(), the next READY task's cpuState is returned
    movl %eax, %esp

    # Restore segment registers from (new) stack
    popl %gs
    popl %fs
    popl %es
    popl %ds
    # Restore general-purpose registers
    popa
    # Skip the 4-byte error code / dummy placeholder
    add $4, %esp
    # Return from interrupt: pops EIP, CS, EFLAGS (resuming task execution)
    iret

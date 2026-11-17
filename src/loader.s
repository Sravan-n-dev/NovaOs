# NovaOS boot entry: Multiboot header + stack + jump into C++
.set MAGIC,    0x1BADB002
.set FLAGS,    (1<<0 | 1<<1)          # align modules, request memory map
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
    .long MAGIC
    .long FLAGS
    .long CHECKSUM

.section .text
.extern kernelMain
.extern callConstructors
.global loader

loader:
    mov $kernel_stack, %esp      # GRUB gives us no usable stack
    push %eax                    # save Multiboot magic across the call
    push %ebx                    # save Multiboot info pointer
    call callConstructors        # run C++ global constructors
    call kernelMain              # args already on stack: (info, magic)
_stop:
    cli
    hlt
    jmp _stop

.section .bss
.space 2*1024*1024               # 2 MiB kernel stack
kernel_stack:

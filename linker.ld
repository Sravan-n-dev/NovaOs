ENTRY(loader)
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386:i386)

SECTIONS
{
    . = 0x0100000;               /* GRUB loads kernel at 1 MiB */

    .text : {
        *(.multiboot)            /* header must be in the first 8 KiB */
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
        start_ctors = .;
        KEEP(*(.init_array*))
        KEEP(*(.ctors))
        end_ctors = .;
    }
    .data : { *(.data*) }
    .bss  : { *(.bss*) *(COMMON) }

    /DISCARD/ : { *(.fini_array*) *(.comment) *(.eh_frame) }
}

SECTIONS
{
    . = 0;
    .data :
    {
        *(.data)
        *(.rodata)
    }

    .text :
    {
        *(.text)
    }

    .bss :
    {
        *(.bss)
    }

    .stab :
    {
        *(.stab)
    }
    
    .stabstr :
    {
        *(.stabstr)
    }

    /DISCARD/ : { *(.comment) *(.eh_frame) }
}

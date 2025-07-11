#ifndef ASM_RISCV_PGTABLE_BITS_H
#define ASM_RISCV_PGTABLE_BITS_H

#define _PAGE_PRESENT   (1 << 0)
#define _PAGE_READ      (1 << 1)    /* Readable */
#define _PAGE_WRITE     (1 << 2)    /* Writable */
#define _PAGE_EXEC      (1 << 3)    /* Executable */
#define _PAGE_USER      (1 << 4)    /* User */
#define _PAGE_GLOBAL    (1 << 5)    /* Global */
#define _PAGE_ACCESSED  (1 << 6)    /* Set by hardware on any access */
#define _PAGE_DIRTY     (1 << 7)    /* Set by hardware on any write */
#define _PAGE_SOFT      (3 << 8)    /* Reserved for software */

#define _PAGE_TABLE     _PAGE_PRESENT
#define _PAGE_PFN_SHIFT 10

#endif
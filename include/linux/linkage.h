#ifndef _LINUX_LINKAGE_H
#define _LINUX_LINKAGE_H

#include <asm/linkage.h>

#define SYM_T_NONE				0

/* Some toolchains use other characters (e.g. '`') to mark new line in macro */
#ifndef ASM_NL
#define ASM_NL		 ;
#endif

/* SYM_T_FUNC -- type used by assembler to mark functions */
#ifndef SYM_T_FUNC
#define SYM_T_FUNC				STT_FUNC
#endif

/* SYM_T_OBJECT -- type used by assembler to mark data */
#ifndef SYM_T_OBJECT
#define SYM_T_OBJECT				STT_OBJECT
#endif

/* SYM_T_NONE -- type used by assembler to mark entries of unknown type */
#ifndef SYM_T_NONE
#define SYM_T_NONE				STT_NOTYPE
#endif

/* SYM_A_* -- align the symbol? */
#define SYM_A_ALIGN				ALIGN
#define SYM_A_NONE				/* nothing */

/* SYM_L_* -- linkage of symbols */
#define SYM_L_GLOBAL(name)			.globl name
#define SYM_L_WEAK(name)			.weak name
#define SYM_L_LOCAL(name)			/* nothing */

#ifndef LINKER_SCRIPT
#define ALIGN __ALIGN
#define ALIGN_STR __ALIGN_STR

#define SYM_ENTRY(name, linkage, align...)		\
	linkage(name) ASM_NL				\
	align ASM_NL					\
	name:

#define SYM_START(name, linkage, align...)		\
	SYM_ENTRY(name, linkage, align)

/* SYM_END -- use only if you have to */
#ifndef SYM_END
#define SYM_END(name, sym_type)				\
	.type name sym_type ASM_NL			\
	.set .L__sym_size_##name, .-name ASM_NL		\
	.size name, .L__sym_size_##name
#endif

#define SYM_CODE_START(name)				\
	SYM_START(name, SYM_L_GLOBAL, SYM_A_ALIGN)

#define SYM_CODE_END(name)				\
	SYM_END(name, SYM_T_NONE)

#define SYM_DATA_START(name)				\
	SYM_START(name, SYM_L_GLOBAL, SYM_A_NONE)

#define SYM_DATA_END(name)				\
	SYM_END(name, SYM_T_OBJECT)

/* SYM_DATA -- start+end wrapper around simple global data */
#ifndef SYM_DATA
#define SYM_DATA(name, data...)				\
	SYM_DATA_START(name) ASM_NL				\
	data ASM_NL						\
	SYM_DATA_END(name)
#endif

#endif

#endif /* _LINUX_LINKAGE_H */

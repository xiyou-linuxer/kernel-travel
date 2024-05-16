/*
 * Generate definitions needed by the preprocessor.
 * This code generates raw asm output which is post-processed
 * to extract and format the required data.
 */

#define __GENERATING_BOUNDS_H
/* Include headers that define the enum constants of interest */
#include <xkernel/types.h>
#include <xkernel/kbuild.h>

void __main(void);

void __main(void)
{ 
	/* The enum constants to put into include/generated/bounds.h */
	/* End of constants */
}

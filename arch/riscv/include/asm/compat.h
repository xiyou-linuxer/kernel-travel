#ifndef __ASM_RISCV_COMPAT_H
#define __ASM_RISCV_COMPAT_H


static inline int is_compat_task(void){
    if (!IS_ENABLED(CONFIG_COMPAT))
		return 0;

	return test_thread_flag(TIF_32BIT);

}


#endif
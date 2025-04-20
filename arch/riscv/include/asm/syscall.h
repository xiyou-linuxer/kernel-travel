#ifndef _SYSCALL_H
#define _SYSCALL_H

#define SYS_PSTR 297

#define SYS_getcwd           17
#define SYS_dup              23
#define SYS_dup2             24
#define __NR3264_fcntl       25
#define SYS_mkdirat          34
#define SYS_unlinkat         35
#define SYS_umount2          39
#define SYS_mount            40
#define SYS_chdir            49
#define SYS_openat           56
#define SYS_close            57
#define SYS_pipe2            59
#define SYS_getdents64       61
#define SYS_read             63
#define SYS_write            64
#define SYS_writev           66
#define __NR3264_sendfile    71
#define SYS_ppoll            73
#define SYS_splice           76
#define SYS_readlinkat       78
#define SYS_fstat            80
#define SYS_exit             93
#define SYS_set_tid_address  96
#define SYS_nanosleep       101
#define SYS_sched_yield     124
#define SYS_kill            129
#define SYS_sigaction       134
#define SYS_sigreturn       139
#define SYS_times           153
#define SYS_uname           160
#define SYS_gettimeofday    169
#define SYS_getpid          172
#define SYS_getppid         173
#define SYS_getgid          176
#define SYS_brk             214
#define SYS_munmap          215
#define SYS_clone           220
#define SYS_execve          221
#define SYS_mmap            222
#define SYS_mprotect        226
#define SYS_wait4           260
#define SYS_statx           291
#define SYS_PCB             298
#define SYS_PP              299
#define SYS_fcntl           __NR3264_fcntl
#define SYS_sendfile        __NR3264_sendfile

#define AT_FDCWD -100
#define AT_OPEN -10

#endif /* _SYSCALL_H */

#ifndef _LINUX_COMPILER_ATTRIBUTTES_H
#define _LINUX_COMPILER_ATTRIBUTTES_H

#define __section(section)              __attribute__((__section__(section)))

#define __noreturn                      __attribute__((__noreturn__))

#define __aligned(x)                    __attribute__((__aligned__(x)))
#define __packed                        __attribute__((__packed__))

# define fallthrough                    __attribute__((__fallthrough__))

#define __weak                          __attribute__((__weak__))

#define __cold                          __attribute__((__cold__))

#define __no_sanitize_address __attribute__((__no_sanitize_address__))

#endif /* _LINUX_COMPILER_ATTRIBUTTES_H */

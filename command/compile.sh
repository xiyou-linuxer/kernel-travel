BIN="init_code"
CFLAGS="-c -g  -mabi=lp64s -D__KERNEL__"
OBJS="../arch/loongarch/kernel/syscall.o "

CC="/opt/gcc-13.2.0-loongarch64-linux-gnu/bin/loongarch64-linux-gnu-gcc"
LD="/opt/gcc-13.2.0-loongarch64-linux-gnu/bin/loongarch64-linux-gnu-ld"

rm *.o
rm init_code
$CC $CFLAGS -I "../arch/loongarch/include/" -I "../include" -o printf.o printf.c
$CC $CFLAGS -I "../arch/loongarch/include/" -I "../include" -o $BIN".o" $BIN".c"
$LD -e main -T program.ld printf.o $BIN".o" $OBJS -o $BIN
#$CC -static -g -o $BIN $BIN.c

xxd -i init_code ../include/xkernel/initcode.h

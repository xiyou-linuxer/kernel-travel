BIN="user_prog"
CFLAGS="-c -g  -mabi=lp64s"
OBJS="../arch/loongarch/kernel/syscall.o "

rm *.o
rm user_prog
/opt/gcc-13.2.0-loongarch64-linux-gnu/bin/loongarch64-linux-gnu-gcc $CFLAGS -I "../arch/loongarch/include/" -I "../include" -o printf.o printf.c
/opt/gcc-13.2.0-loongarch64-linux-gnu/bin/loongarch64-linux-gnu-gcc $CFLAGS -I "../arch/loongarch/include/" -I "../include" -o $BIN".o" $BIN".c"
/opt/gcc-13.2.0-loongarch64-linux-gnu/bin/loongarch64-linux-gnu-ld -e main -T program.ld printf.o $BIN".o" $OBJS -o $BIN

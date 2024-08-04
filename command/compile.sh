BIN="user_prog"
#CFLAGS="-c -g  -mabi=lp64s"
OBJS="../arch/loongarch/kernel/syscall.o "

CC="/opt/gcc-13.2.0-loongarch64-linux-gnu/bin/loongarch64-linux-gnu-gcc"
LD="/opt/gcc-13.2.0-loongarch64-linux-gnu/bin/loongarch64-linux-gnu-ld"

# CC="../cross-tools/bin/loongarch64-unknown-linux-gnu-gcc"
# LD="../cross-tools/bin/loongarch64-unknown-linux-gnu-ld"

rm *.o
rm user_prog
#$CC $CFLAGS -I "../arch/loongarch/include/" -I "../include" -o printf.o printf.c
#$CC $CFLAGS -I "../arch/loongarch/include/" -I "../include" -o $BIN".o" $BIN".c"
$CC -static -g -o $BIN $BIN.c
#$LD -e main -T program.ld $BIN".o" $OBJS -o $BIN


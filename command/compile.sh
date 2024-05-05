BIN="user_prog"
CFLAGS="-c -g  -mabi=lp64s"
OBJS="../arch/loongarch/kernel/syscall.o"

../cross-tools/bin/loongarch64-unknown-linux-gnu-gcc $CFLAGS -I "../arch/loongarch/include/" -I "../include" -o $BIN".o" $BIN".c"
../cross-tools/bin/loongarch64-unknown-linux-gnu-ld -e main -T program.ld $BIN".o" $OBJS -o $BIN

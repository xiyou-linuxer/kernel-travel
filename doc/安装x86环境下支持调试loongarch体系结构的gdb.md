# 安装x86环境下支持调试loongarch体系结构的gdb

1. docker中下载并解压：

```bash
wget http://ftp.gnu.org/gnu/gdb/gdb-12.1.tar.gz
tar -zxvf gdb-12.1.tar.gz 
cd gdb-12.1
mkdir build
cd build
../configure --prefix=/usr --target=loongarch64-unknown-linux-gnu
```

1. 编译

```bash
make -j16
make install
```

注意：这里一定会报错，我记得是：缺失了 expect 工具

```shell
apt-get install expect
```

------------------------------------------------
在`gdb-12.1/build/gdb`目录下运行`gdb`可执行文件，看到下面信息说明安装成功：

```shell
GNU gdb (GDB) 12.1
Copyright (C) 2022 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
Type "show copying" and "show warranty" for details.
This GDB was configured as "--host=x86_64-pc-linux-gnu --target=loongarch64-unknown-linux-gnu".
```
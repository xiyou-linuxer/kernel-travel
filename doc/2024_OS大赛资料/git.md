# git 下载编译

手写实现了一个简易版的git:
能完成task1、task2,路径：`./scripts/my_git`。在线下赛现场完成的，git链接：`git@github.com:zhendewokusi/my_git.git`，十分简单的实现。

实现了`git add`,`git commit`,`git checkout`,`git log`，`git reflog`命令。

`.git`仓库拓扑：
```
➜  .git git:(main) tree
.
├── HEAD
├── index
├── objects
│   ├── 10f99f17
│   └── 434eb2fc
└── refs

3 directories, 4 files
```

`object`文件夹用于存放文件，修改等。
`refs`文件夹用于存储tag。

主机配置交叉编译版本的 `zlib`：

```bash
wget https://github.com/madler/zlib/archive/refs/heads/develop.zip
cd zlib-develop
export CROSS_COMPILE=/opt/gcc-13.2.0-loongarch64-linux-gnu/bin/loongarch64-linux-gnu-
export CC=${CROSS_COMPILE}gcc
export CXX=${CROSS_COMPILE}g++
export AR=${CROSS_COMPILE}ar
export LD=${CROSS_COMPILE}ld
export LDFLAGS="-static"
export CFLAGS="--sysroot=/opt/gcc-13.2.0-loongarch64-linux-gnu/sysroot -I/opt/gcc-13.2.0-loongarch64-linux-gnu/include"
export LDFLAGS="--sysroot=/opt/gcc-13.2.0-loongarch64-linux-gnu/sysroot -L/opt/gcc-13.2.0-loongarch64-linux-gnu/lib"
./configure --build=x86_64-linux-gnu --host=loongarch64-linux-gnu
make -j 16
make install DESTDIR=/opt/gcc-13.2.0-loongarch64-linux-gnu/ includedir=include
```

编译生成交叉编译版本的`git`：
```bash
wget https://github.com/git/git/archive/refs/tags/v2.17.0-rc1.zip

unzip v2.17.0-rc1.zip

export CROSS_COMPILE=/opt/gcc-13.2.0-loongarch64-linux-gnu/bin/loongarch64-linux-gnu-
export CC=${CROSS_COMPILE}gcc
export CXX=${CROSS_COMPILE}g++
export AR=${CROSS_COMPILE}ar
export LD=${CROSS_COMPILE}ld
export LDFLAGS="-static"
export CFLAGS="--sysroot=/opt/gcc-13.2.0-loongarch64-linux-gnu/sysroot -I/opt/gcc-13.2.0-loongarch64-linux-gnu/include"
export LDFLAGS="--sysroot=/opt/gcc-13.2.0-loongarch64-linux-gnu/sysroot -L/opt/gcc-13.2.0-loongarch64-linux-gnu/lib"

./configure --build=x86_64-linux-gnu --host=loongarch64-linux-gnu

```

适配了`zlib`后，关于`zlib.h`相关的报错没有了，但是还有很多编译器相关报错，不清楚如何解决
报错：
```err
23 个结果 - 1 个文件

config.log:
   72  configure:3267: /opt/gcc-13.2.0-loongarch64-linux-gnu/bin/loongarch64-linux-gnu-gcc -V >&5
   73: loongarch64-linux-gnu-gcc: error: unrecognized command-line option '-V'
   74: loongarch64-linux-gnu-gcc: fatal error: no input files
   75  compilation terminated.

   77  configure:3267: /opt/gcc-13.2.0-loongarch64-linux-gnu/bin/loongarch64-linux-gnu-gcc -qversion >&5
   78: loongarch64-linux-gnu-gcc: error: unrecognized command-line option '-qversion'; did you mean '--version'?
   79: loongarch64-linux-gnu-gcc: fatal error: no input files
   80  compilation terminated.

   82  configure:3267: /opt/gcc-13.2.0-loongarch64-linux-gnu/bin/loongarch64-linux-gnu-gcc -version >&5
   83: loongarch64-linux-gnu-gcc: error: unrecognized command-line option '-version'
   84: loongarch64-linux-gnu-gcc: fatal error: no input files
   85  compilation terminated.

  155  conftest.c: In function 'main':
  156: conftest.c:50:21: error: expected expression before ')' token
  157     50 | if (sizeof ((size_t)))

  243  configure:4502: /opt/gcc-13.2.0-loongarch64-linux-gnu/bin/loongarch64-linux-gnu-gcc -V >&5
  244: loongarch64-linux-gnu-gcc: error: unrecognized command-line option '-V'
  245: loongarch64-linux-gnu-gcc: fatal error: no input files
  246  compilation terminated.

  248  configure:4502: /opt/gcc-13.2.0-loongarch64-linux-gnu/bin/loongarch64-linux-gnu-gcc -qversion >&5
  249: loongarch64-linux-gnu-gcc: error: unrecognized command-line option '-qversion'; did you mean '--version'?
  250: loongarch64-linux-gnu-gcc: fatal error: no input files
  251  compilation terminated.

  253  configure:4502: /opt/gcc-13.2.0-loongarch64-linux-gnu/bin/loongarch64-linux-gnu-gcc -version >&5
  254: loongarch64-linux-gnu-gcc: error: unrecognized command-line option '-version'
  255: loongarch64-linux-gnu-gcc: fatal error: no input files
  256  compilation terminated.

  269  configure:4873: /opt/gcc-13.2.0-loongarch64-linux-gnu/bin/loongarch64-linux-gnu-gcc -o conftest --sysroot=/opt/gcc-13.2.0-loongarch64-linux-gnu/sysroot -I/opt/gcc-13.2.0-loongarch64-linux-gnu/include  --sysroot=/opt/gcc-13.2.0-loongarch64-linux-gnu/sysroot -L/opt/gcc-13.2.0-loongarch64-linux-gnu/lib -R / conftest.c  >&5
  270: loongarch64-linux-gnu-gcc: error: unrecognized command-line option '-R'
  271  configure:4873: $? = 1

  329  /opt/gcc-13.2.0-loongarch64-linux-gnu/bin/../lib/gcc/loongarch64-linux-gnu/13.2.0/../../../../loongarch64-linux-gnu/bin/ld: cannot find -lcurl: No such file or directory
  330: collect2: error: ld returned 1 exit status
  331  configure:5607: $? = 1

  353  | 
  354: | /* Override any GCC internal prototype to avoid an error.
  355  |    Use char because int might match the return type of a GCC

  374  /opt/gcc-13.2.0-loongarch64-linux-gnu/bin/../lib/gcc/loongarch64-linux-gnu/13.2.0/../../../../loongarch64-linux-gnu/bin/ld: cannot find -lexpat: No such file or directory
  375: collect2: error: ld returned 1 exit status
  376  configure:5751: $? = 1

  398  | 
  399: | /* Override any GCC internal prototype to avoid an error.
  400  |    Use char because int might match the return type of a GCC

  423  /opt/gcc-13.2.0-loongarch64-linux-gnu/bin/../lib/gcc/loongarch64-linux-gnu/13.2.0/../../../../loongarch64-linux-gnu/bin/ld: cannot find -lz: No such file or directory
  424: collect2: error: ld returned 1 exit status
  425  configure:5901: $? = 1

  468  configure:6058: result: yes
  469: configure:6124: checking for hstrerror
  470  configure:6124: /opt/gcc-13.2.0-loongarch64-linux-gnu/bin/loongarch64-linux-gnu-gcc -o conftest --sysroot=/opt/gcc-13.2.0-loongarch64-linux-gnu/sysroot -I/opt/gcc-13.2.0-loongarch64-linux-gnu/include  --sysroot=/opt/gcc-13.2.0-loongarch64-linux-gnu/sysroot -L/opt/gcc-13.2.0-loongarch64-linux-gnu/lib conftest.c  >&5

  504  configure:6421: checking whether iconv omits bom for utf-16 and utf-32
  505: configure:6435: error: in '/home/yuanfang/Downloads/git-master':
  506: configure:6437: error: cannot run test program while cross compiling
  507  See 'config.log' for more details

  533  ac_cv_func_alloca_works=yes
  534: ac_cv_func_hstrerror=yes
  535  ac_cv_func_inet_ntop=yes

```

尝试修改，没有结果，直接修改Makefile中的CC，AR,以及 LDFLAGS 和 CFLAGS 为对应的，编译发现缺少 `curl`库，安装龙芯的 `curl` 库，`curl`库依赖龙芯的`libpsl`,`libpsl`又依赖两个 lib 库....

```bash
wget https://github.com/rockdaboot/libpsl/archive/refs/heads/master.zip
unzip master.zip
cd libpsl-master
mkdir build
cd build
make -j 16
make install DESTDIR=/opt/gcc-13.2.0-loongarch64-linux-gnu/ includedir=include
```
```bash
wget https://github.com/curl/curl/archive/refs/heads/master.zip
unzip curl-master.zip
cd curl-master
mkdir build
cd build
make -j 16
make install DESTDIR=/opt/gcc-13.2.0-loongarch64-linux-gnu/ includedir=include
```



直接修改 git 的 Makefile
```Makefile
CROSS_COMPILE=/opt/gcc-13.2.0-loongarch64-linux-gnu/bin/loongarch64-linux-gnu-

CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar

CFLAGS = -g -O2 -Wall --sysroot=/opt/gcc-13.2.0-loongarch64-linux-gnu/sysroot -I/opt/gcc-13.2.0-loongarch64-linux-gnu/include
LDFLAGS = --sysroot=/opt/gcc-13.2.0-loongarch64-linux-gnu/sysroot -L/opt/gcc-13.2.0-loongarch64-linux-gnu/lib

```
编译安装。

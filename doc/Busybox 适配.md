# Busybox 适配


### busybox 编译

#### 关于工具链muslc
```
编译 LoongArch musl libc库
git clone https://github.com/LoongsonLab/oscomp-musl-1.2.4.git

# 保证交叉编译器已经正确安装
./configure --target=loongarch64-linux-gnu CFLAGS="-mabi=lp64d" --prefix=/opt/loongarch-muslc


make -j 16
make install -j 16
```
在.bashrc里修改PATH添加相关目录
```
#添加交叉编译工具到PATH
export PATH=$PATH:[your tools]
```

## 下载Linux引入相关头文件
```
make ARCH=loongarch INSTALL_HDR_PATH=../temp headers_install
```
更改busybox的Makefile
```
CFLAGS		:= $(CFLAGS) -I /**/include
```

### 修改编译选项
```
make defconfig
CONFIG_TC=y ==> CONFIG_TC=n

nvim scripts/kconfig/lxdialog/check-lxdialog.sh
main ==> int main()

make menuconfig 设置静态编译
make CC=musl-gcc CROSS_COMPILE=loongarch64-linux-gnu- -j 16
make CC=musl-gcc  CROSS_COMPILE=loongarch64-linux-gnu- install -j 16
```

### 制作包含busybox的fat32镜像

### 将busybox 读入内存
像之前读sdcard那样，把sdcard替换为自己制作的。
fat32.img复制到/tmp/qemu，然后调整runqemu.sh
```
sudo docker cp fat32.img os-contest:/tmp/qemu/fat32.img

>>runqemu.sh:
-hdb fat32.img
```


## 适配日记

运行时出现
```
Error: unkown opcode. 00000000004108b0: 0xfa1e0ff3
Error: unkown opcode. 90000000901ba79c: 0x0
```

1.busybox 编译出的机器码无法被识别
验证方法：如果确实运行到那一步，并且当前机器码与busybox中一致，则说明是这个原因。
验证结果：跳转的地方是0x00000000004108b0，确实运行到那里了
现在查看是否与Busybox中一致。
查看该虚拟地址是从Busybox的哪里移过来的
打印得到的结果:
```
vaddr=400000:p_offset=0
vaddr=401000:p_offset=1000
vaddr=5c9000:p_offset=1c9000
vaddr=6372e0:p_offset=2362e0
```
推得，vaddr=4108b0,busybox的offset=0x108b0
检查了一下，确实一毛一样：
```
000108b0: f30f 1efa 31ed 4989 d15e 4889 e248 83e4
```

仔细想来，这个busybox我在电脑上运行了一下是好的。然鹅，我的电脑并非龙芯架构，所以反而是错的。
检查一下
```
busybox: ELF 64-bit LSB executable, x86-64, version 1 (GNU/Linux)
```
果真如此
检查修改busybox交叉编译流程
发现make install 也需要加CROSS_COMPILE,或许它重新又编译了一遍？
重试

出现
```
!!!!! error !!!!!
filename:drivers/../lib/kernel/bitmap.c
line:52
function:bitmap_set
```
是内存管理的问题。
经检查，启动进程前分配的虚拟页是0x120000000，超出当前内核虚拟内存范围0x40000000
改成0x8000000000分配栈时出现了一个特别大的数
原因是(1<<(9+9+9+12))会被截断变成0,改成1UL就好了。


```
jump to proc... at 200004a4
Warning: page_size is 0
```
1.加载的虚拟地址未完全有效关联物理地址

/********* 正确原因 ************/
2.跳错地方了，真的入口不在那
查看了下busybox`Entry point 0x1200004a4`。
比较两个数据应该是在哪里被截断了,找到在这个函数
```
int sys_exeload(const char *path)
```

好吧，改了还有类似的问题：
```
jump to proc... at 1200004a4
Warning: page_size is 0
Error: unkown opcode. 90000000901b9d9c: 0x0
Error: unkown opcode. 90000000901ba79c: 0x0
qemu-system-loongarch64: terminating on signal 2
```
gdb进去仔细查看了下，已经开始成功执行代码了，应该是执行到哪步突然出的错。
进一步的观察：
1.是在加载数据时出错的
通过读出寄存器的数值，查看加载的内存地址具体是多少,然后追查为何那里会有问题。
读入地址：0x120284178
检查下是否在phdr所指示的范围内？
情况一：在范围内，但是没分配到内存
情况二：在范围内也分配到了，但是后续某些操作干扰了使得那块内存失效。
情况三：不在指示范围内

打印的范围：
```
range:120000000 - 120210ef4
range:1202150f8 - 12027e1c0
```
看样子是情况三
查看busybox中相应位置是个什么段
检查一下段的范围，发现第二个段的范围应该是`0x00000001202150f8 - 0x120285090`
原因是我把p_filesz与p_memsz混为一谈了。有些在内存某些地方是被占用的，只不过他们不在文件中罢了


```
Error: unkown opcode. 90000000901b9d9c: 0x0
Error: unkown opcode. 90000000901ba79c: 0x0
```

更多详细细节:
在执行
```
ldptr t0 0
```
其中t0=0x4e03043017fc0328
转移到
```
0x90000000901b1004 <exception_handlers+4100>
```

首先这个t0的值明显不对劲，检查一下初始赋值，查清为0
然后这个<exception_handlers+4100>具体是什么，查明是地址错误
所以是Busybox程序自己的问题？亦或者是某些内存没有效加载程序？
进一步调查结果显示
```
0x1200004bc     ld.d            $a1, $sp, 0
```
起因是这里$sp=0x8000000000的时候，加载了不存在的东西
其实应该是要有东西的，就是argv和argc，但我没写好，现在补上
1.制作程序入口向栈中推入argv和argc，其他照常
本来是寄存器传参，没有用到栈
这里既然是赋值给a1，意味着原来直接给a1赋值传参的思路是错的，这边用栈转参

2.在内存空间内专门留一块存放argv
改变一下内存格局，最上面放argv，下面是栈。

```
---------- stack
argv[argc-1]
......
argv[1]
argv[0]
```

增加了，但是没什么用，看来很可能不是这个原因
在busybox的init_main中增添了printf，但并没打印出任何东西就死了，那时还没有用到argv。难道是init_main之前还有一段程序？
查看符号表，没有符号表，CFLAGS添加-g，还是没有。
看了下busybox的makefile，发现默认是stripped，修改Makefile,添加SKIP_STRIP = y

```
00000001200d2e98 T init_main
```
现在的Entry在 0x12010e38c
```
000000012010df00 T __trunctfdf2
000000012010e398 T _start
```
怪了，应该跳到_start吧，readelf读出来的也是000000012010e398 
哦，原来是忘记重新制作fat32镜像了，犯了个低级错误，尴尬。。。

gdb看一下具体在哪死了
大概这个范围
```
0x00000001200e4224 in ?? ()
0x00000001200e4228 in ?? ()
0x00000001200e422c in ?? ()
0x00000001200e4230 in ?? ()
0x00000001200e4234 in ?? ()
0x00000001200e4238 in ?? ()
0x00000001200e423c in ?? ()
0x00000001200e3fa0 in ?? ()
```

查一下符号表
```
00000001200e4210 T __libc_start_main
00000001200e426c T clearenv
00000001200e42b8 T getenv
```
最终的死亡地点在
```
00000001200e3fa0 T __init_libc
```
离谱，我重新试了一遍，这次居然没出现这问题，一切正常地执行到系统调用。
又试了几次也是这样，那前面是怎么回事？
算了不管了，继续把。

第一个系统调用号为96
于musl源码中可找到
`#define __NR_set_tid_address            96`
搜索了一下干什么的，简单实现了下


接下来：291
statx，这个应该实现过了，看看哪里出问题了
没有这个东西：/etc/busybox.conf
当前fat32.img中只放入了busybox，busybox需要读取这个作为配置，所以我还得把这个带上
找了下我本机就没有这个文件，但是Busybox还能运行，说明这个文件并非绝对必要，所以是sys_statx缺陷，修改成如果没有这个文件就直接退出

为了方便，在do_syscall执行前后增添打印系统调用号,方便定位哪些系统调用需要修改



```
!!!!! error !!!!!
filename:mm/memory.c
line:47
function:free_page
```

错误原因是
```
free_page(*ptep&0xfffffffffffff000);
```
*ptep 给出一个明显不合理的值

```
uint64_t* ptep = reverse_pte_ptr(pdir,vaddr);
```
1.reverse_pte_ptr这个函数写的不对
2.页表本身数据不对



#!/bin/bash

PWD="`pwd`"

TOOLCHAINS=/home/huloves/repositories/os-loongson/toolchains/loongson-gnu-toolchain-x86_64-loongarch64-linux-gnu/bin/loongarch64-linux-gnu-

function distclean()
{
	make ARCH=loongarch distclean
}

function defconfig()
{
	make ARCH=loongarch CROSS_COMPILE=$TOOLCHAINS defconfig
}

function menuconfig()
{
	make ARCH=loongarch CROSS_COMPILE=$TOOLCHAINS menuconfig
}

function run()
{
	qemu-system-loongarch64 -nographic \
	  -machine virt \
          -m 4G -cpu la464 -smp 1 \
          -bios ../QEMU_EFI.fd \
	  -kernel ./arch/loongarch/boot/Image
}

function rungdb()
{
	qemu-system-loongarch64 -nographic \
	  -machine virt \
          -m 4G -cpu la464 -smp 1 \
          -bios ../QEMU_EFI.fd \
	  -kernel ./arch/arm64/boot/Image \
	  -S -gdb tcp::1234
}

function image()
{
	make ARCH=loongarch CROSS_COMPILE=$TOOLCHAINS -j1 Image
}

function gdb()
{
	make ARCH=loongarch CROSS_COMPILE=$TOOLCHAINS -j1 Image && rungdb && exit
}

function all()
{
	make ARCH=loongarch CROSS_COMPILE=$TOOLCHAINS Image && run && exit
}

#
# 将字符串转换为函数，并调用函数
#
function call_sub_cmd()
{
	#
	# 通过第一个参数，获得想要调用的函数名
	# 例如 check 函数
	#
	func=$1
	#
	# 函数名不支持”-“，因此将参数中的”-“转换为”_“
	#
	func=${func//-/_}
	#
	# 从参数列表中移除第一个参数，例如 check，将剩余的参数传给函数
	#
	shift 1
	eval "$func $*"
}

#
# 主函数
#
function main()
{
	#
	# 如果没有任何参数，默认调用all函数
	#
	if [ $# -eq 0 ]; then
		all
		exit 0
	fi

	#
	# 带参数运行，看看相应的函数是否存在
	#
	SUB_CMD=$1
	type ${SUB_CMD//-/_} > /dev/null 2>&1
	#
	# 要调用的子函数不存在，说明用法错误
	#
	if [ $? -ne 0 ]; then
		usage
		exit
	else
		#
		# 要调用的子函数存在，执行子函数
		#
		shift 1;
		call_sub_cmd $SUB_CMD $*
		exit $?
	fi

	usage
}

#
# 调用主函数
#
main $*

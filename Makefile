
include ./Makefile_back

all:
	mv arch/loongarch/boot/Image kernel.bin
	md5sum kernel.bin
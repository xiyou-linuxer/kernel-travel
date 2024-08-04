#!/bin/bash
DISK=/tmp/disk
QEMU=/tmp/qemu/bin/qemu-system-loongarch64
[ -e $DISK ] || { truncate -s 32M $DISK;echo -e 'n\n\n\n\n\n\nw\nq\n'| fdisk /tmp/disk; }

ls2k()
{
BIOS=/tmp/qemu/share/qemu/gzrom-dtb-la2k.bin
BIOS=/tmp/qemu/2k1000/u-boot-with-spl.bin
DEBUG_GMAC_PHYAD=0 $QEMU -M ls2k -serial stdio -serial vc -drive if=pflash,file=$BIOS  -m 1024 -device usb-kbd,bus=usb-bus.0 -device usb-tablet,bus=usb-bus.0  -device usb-storage,drive=udisk -drive if=none,id=udisk,file=$DISK -net nic,model=virtio-net-pci -net tap,ifname=tap0,script=no,downscript=no -vnc :0  -D /tmp/qemu.log -s "$@" 2>&1 -hda /tmp/qemu/2k1000/2kfs.img -hdb /sdcard-loongarch.img
}

ls2k "$@"
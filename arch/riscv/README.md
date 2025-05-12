qemu 启动命令
```shell
    qemu-system-riscv64 \
      -machine virt \
      -kernel kernel-travel/arch/riscv/boot/Image \
      -m 1G -nographic -smp 1 \
      -bios default \
      -drive file=busybox-1.36.1/rootfs_kernel.ext2,if=none,format=raw,id=x0 \
      -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0 \
      -no-reboot \
      -device virtio-net-device,netdev=net \
      -netdev user,id=net \
      -rtc base=utc \
      -append "root=/dev/vda rw console=ttyS0"
```

B:gdb-multiarch kernel-travel/vmlinux

set arch riscv:rv64
target remote :1234

add-symbol-file kernel-travel/vmlinux 0x80200000 -s .text 0xffffffff80000000

objdump -h kernel-travel/vmlinux

x/10i $pc
disas

readelf

riscv64-linux-musl-readelf -h kernel-rv riscv64-linux-musl-readelf -a kernel-rv

dd if=/dev/zero of=./rootfs.ext2 bs=1M count=512

losetup -f
sudo losetup /dev/loop2 ./rootfs.ext2
sudo mkfs.ext2 -q /dev/loop2
mkdir /tmp/rootfs
sudo mount -o loop ./rootfs.ext2 /tmp/rootfs   #挂载
sudo rsync -avr _install/* /tmp/rootfs
sudo cd /tmp/rootfs && mkdir -p proc sys dev etc/init.d  

sudo vim etc/init.d/rcS
#!/bin/sh
mount -t proc none /proc
mount -t sysfs none /sys
/sbin/mdev -s

sudo chmod +x etc/init.d/rcS
sudo umount /tmp/rootfs
```shell
  qemu-system-riscv64 \
      -machine virt \
      -kernel arch/riscv/boot/Image \
      -m 1G -nographic -smp 1 \
      -bios default \
      -drive file=../busybox-1.36.1/rootfs.ext2,if=none,format=raw,id=x0 \
      -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0 \
      -no-reboot \
      -device virtio-net-device,netdev=net \
      -netdev user,id=net \
      -rtc base=utc \
      -append "root=/dev/vda rw console=ttyS0"
```

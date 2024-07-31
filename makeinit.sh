cd command
./compile.sh
cd ..
xxd  -n init_code -i command/user_prog include/xkernel/initcode.h
make all
sudo docker cp kernel.bin os-contest:/srv/tftp/kernel.bin
sudo docker cp vmlinux os-contest:/vmlinux


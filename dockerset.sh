apt-get install bridge-utils uml-utilities net-tools tftpd-hpa
apt-get update
apt-get install vim

ip link add br0 type bridge
ip tuntap add dev tap0 mode tap
brctl addif br0 tap0
ifconfig br0 0.0.0.0 promisc up
ifconfig tap0 0.0.0.0 promisc up
ifconfig br0 10.0.0.1 netmask 255.255.255.0

cd /tmp/qemu/2k1000
./create_qemu_img.sh
cd ..

service tftpd-hpa start

echo "所有操作已完成。"

#sudo bpi-bootsel SD/bpi-m64/100MB/u-boot-with-dtb-bpi-m64-720p-8k.img.gz $1
sudo gunzip -c SD/bpi-m64/100MB/BPI-M64-LCD7-linux4.4-8k.img.gz | dd of=$1 bs=1024 seek=8
sync
cd SD/bpi-m64
sudo bpi-update -d $1

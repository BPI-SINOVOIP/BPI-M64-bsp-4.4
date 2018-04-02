#!/bin/bash
# (c) 2015, Leo Xu <otakunekop@banana-pi.org.cn>
# Build script for BPI-M2P-BSP 2016.03.02

T=
MACH="sun50iw1p1"
BPIMACH="sun50iw1p1 sun8iw7p1 sun8iw11p1"
BPIMACH="tulip-m64 tulip-noma"
BOARD="BPI-M64-720P"
NEWBOARD=$1
board=
kernel=
BOOT_PACK_P=
MODE=$2



if [[ $NEWBOARD != "" ]]; then BOARD=$NEWBOARD ; fi


echo "top dir $T"

cp_download_files()
{
SD="$T/SD/$board"
U="${SD}/100MB"
B="${SD}/BPI-BOOT"
R="${SD}/BPI-ROOT"
	#
	## clean SD dir.
	#
	rm -rf $SD
	#
	## create SD dirs (100MB, BPI-BOOT, BPI-ROOT) 
	#
	mkdir -p $SD
	mkdir -p $U
	mkdir -p $B
	mkdir -p $R
	#
	## copy files to 100MB
	#
	cp -a $T/out/100MB/* $U
	#cp -a $T/u-boot-sunxi/out/*.img.gz $U
	#
	## copy files to BPI-BOOT
	#
	mkdir -p $B/bananapi/${board}/linux4.4
	cp -a $BOOT_PACK_P/* $B/bananapi/${board}/linux4.4
	cp -a $T/linux-sunxi/arch/arm64/boot/uImage $B/bananapi/${board}/linux4.4/uImage

	#
	## copy files to BPI-ROOT
	#
	mkdir -p $R/usr/lib/u-boot/bananapi/${board}
	cp -a $U/*.gz $R/usr/lib/u-boot/bananapi/${board}/
	rm -rf $R/lib/modules
	mkdir -p $R/lib/modules
	cp -a $T/linux-sunxi/output/lib/modules/${kernel} $R/lib/modules
	#
	## create files for bpi-tools & bpi-migrate
	#
	# BPI-BOOT
	(cd $B ; tar czvf $SD/BPI-BOOT-${board}-linux4.4.tgz .)
	# BPI-ROOT: kernel modules
	#(cd $R ; tar czvf $SD/${kernel}.tgz lib/modules)
	(cd $R ; tar czvf $SD/${kernel}-net.tgz lib/modules/${kernel}/kernel/net)
	(cd $R ; mv lib/modules/${kernel}/kernel/net $R/net)
	(cd $R ; tar czvf $SD/${kernel}.tgz lib/modules)
	(cd $R ; mv $R/net lib/modules/${kernel}/kernel/net)
	# BPI-ROOT: BOOTLOADER
	(cd $R ; tar czvf $SD/BOOTLOADER-${board}-linux4.4.tgz usr/lib/u-boot/bananapi)


	return #SKIP
}

list_boards() {
	cat <<-EOT
	NOTICE:
	new build.sh default select $BOARD and pack all boards
	supported boards:
	EOT
	for MACH in $BPIMACH ; do
		(cd sunxi-pack/allwinner/$MACH/configs ; ls -1d BPI*)
	done
	echo
}

list_boards

if [ -z "$BOARD" ]; then
        echo -e "\033[31mNo BOARD select\033[0m"
	echo
	echo -e "\033[31m# ./build.sh <board> <mode> \033[0m"
	exit 1
fi

./configure $BOARD

if [ -f env.sh ] ; then
	. env.sh
fi

T="$TOPDIR"
case $BOARD in
  BPI-M2P*)
    board="bpi-m2p"
#    kernel="3.4.113-BPI-M2P-Kernel"
    kernel="4.4.55-BPI-M2P-Kernel"
    BOOT_PACK_P=$T/sunxi-pack/chips/${MACH}/configs/default/linux4.4
    ;;
  BPI-M2Z*)
    board="bpi-m2z"
#    kernel="3.4.113-BPI-M2Z-Kernel"
    kernel="4.4.55-BPI-M2P-Kernel"
    BOOT_PACK_P=$T/sunxi-pack/chips/${MACH}/configs/${BOARD}/linux4.4
    ;;
  BPI-M2U*)
    board="bpi-m2u"
    kernel="4.4.55-BPI-M2P-Kernel"
    BOOT_PACK_P=$T/sunxi-pack/chips/${MACH}/configs/${BOARD}/linux4.4
    ;;
  BPI-M64*)
    board="bpi-m64"
    kernel="4.4.89-BPI-M64-Kernel"
    TARGET_PLATFORM="tulip"
    TARGET_PRODUCT="tulip-m64"
    BOOT_PACK_P=$T/sunxi-pack/allwinner/${TARGET_PRODUCT}/configs/default/linux4.4
    ;;
  BPI-R18*)
    board="bpi-m64"
    kernel="4.4.89-BPI-M64-Kernel"
    TARGET_PLATFORM="tulip"
    TARGET_PRODUCT="tulip-m64"
    BOOT_PACK_P=$T/sunxi-pack/allwinner/${TARGET_PRODUCT}/configs/default/linux4.4
    ;;
  *)
    board=$(echo $BOARD | tr '[A-Z]' '[a-z]')
#    kernel="3.4.113-${BOARD}-Kernel"
    kernel="4.4.55-${BOARD}-Kernel"
    BOOT_PACK_P=$T/sunxi-pack/chips/${MACH}/configs/${BOARD}/linux4.4
    ;;
esac

echo "This tool support following building mode(s):"
echo "--------------------------------------------------------------------------------"
echo "	1. Build all, uboot and kernel and pack to download images."
echo "	2. Build uboot only."
echo "	3. Build kernel only."
echo "	4. kernel configure."
echo "	5. Pack the builds to target download image, this step must execute after u-boot,"
echo "	   kernel and rootfs build out"
echo "	6. update files for SD"
echo "	7. Clean all build."
echo "--------------------------------------------------------------------------------"

if [ -z "$MODE" ]; then
	read -p "Please choose a mode(1-7): " mode
	echo
else
	mode=1
fi

if [ -z "$mode" ]; then
        echo -e "\033[31m No build mode choose, using Build all default   \033[0m"
        mode=1
fi

echo -e "\033[31m Now building...\033[0m"
echo
case $mode in
	1) make && 
	   make pack && 
	   cp_download_files;;
	2) make u-boot;;
	3) make kernel;;
	4) make kernel-config;;
	5) make pack;;
	6) cp_download_files;;
	7) make clean;;
esac
echo

echo -e "\033[31m Build success!\033[0m"
echo
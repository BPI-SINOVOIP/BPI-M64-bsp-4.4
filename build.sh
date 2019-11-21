#!/bin/bash
# (c) 2015, Leo Xu <otakunekop@banana-pi.org.cn>
# Build script for BPI-M2P-BSP 2016.03.02

MACH="sun50iw1p1"
BOARD="BPI-M64-720P"
mode=$1

list_boards() {
	cat <<-EOT
	NOTICE:
	new build.sh default select $BOARD and pack all boards
	supported boards:
	EOT
	(cd sunxi-pack/$MACH/configs ; ls -1d BPI*)
}

list_boards
./configure $BOARD

if [ -f env.sh ] ; then
	. env.sh
fi

echo "This tool support following building mode(s):"
echo "--------------------------------------------------------------------------------"
echo "	1. Build all, uboot and kernel and pack to download images."
echo "	2. Build uboot only."
echo "	3. Build kernel only."
echo "	4. kernel configure."
echo "	5. Pack the builds to target download image, this step must execute after u-boot,"
echo "	   kernel and rootfs build out"
echo "	6. Update local build to SD with bpi image flashed"
echo "	7. Clean all build."
echo "--------------------------------------------------------------------------------"

if [ -z "$mode" ]; then
	read -p "Please choose a mode(1-7): " mode
fi

case "$mode" in
	[!1-7])
	    echo -e "\033[31m Invalid mode, exit   \033[0m"
	    exit 1
	    ;;
	*)
	    ;;
esac

echo -e "\033[31m Now building...\033[0m"
echo
case $mode in
	1) make && 
	   make pack;;
	2) make u-boot;;
	3) make kernel;;
	4) make kernel-config;;
	5) make pack;;
	6) make install;;
	7) make clean;;
esac
echo

echo -e "\033[31m Build success!\033[0m"
echo

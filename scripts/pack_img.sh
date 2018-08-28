#!/bin/bash
#
# pack/pack
# (c) Copyright 2013 - 2016 Allwinner
# Allwinner Technology Co., Ltd. <www.allwinnertech.com>
# James Deng <csjamesdeng@allwinnertech.com>
# Trace Wong <wangyaliang@allwinnertech.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#set -x

echo "BPI: $0 $*"
echo "BPI: BOARD=$BOARD"


############################ Notice #####################################
# a. Some config files priority is as follows:
#    - xxx_${platform}.{cfg|fex} > xxx.{cfg|fex}
#    - ${chip}/${board}/*.{cfg|fex} > ${chip}/default/*.{cfg|fex}
#    - ${chip}/default/*.cfg > common/imagecfg/*.cfg
#    - ${chip}/default/*.fex > common/partition/*.fex
#  e.g. sun8iw7p1/configs/perf/image_linux.cfg > sun8iw7p1/configs/default/image_linux.cfg
#       > common/imagecfg/image_linux.cfg > sun8iw7p1/configs/perf/image.cfg
#       > sun8iw7p1/configs/default/image.cfg > common/imagecfg/image.cfg
#
# b. Support Nor storages rule:
#    - Need to create sys_partition_nor.fex or sys_partition_nor_${platform}.fex
#    - Add "{filename = "full_img.fex",     maintype = "12345678", \
#      subtype = "FULLIMG_00000000",}" to image[_${platform}].cfg
#
# c. Switch uart port
#    - Need to add your chip configs into ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_pin
#    - Call pack with 'debug' parameters

#add for debug by zqb
enable_pause=0

function get_char()
{
	SAVEDSTTY=`stty -g`
	stty -echo
	stty cbreak
	dd if=/dev/tty bs=1 count=1 2> /dev/null
	stty -raw
	stty echo
	stty $SAVEDSTTY
}

function pause()
{
	if [ "x$1" != "x" ] ;then
		echo $1
	fi
	if [ $enable_pause -eq 1 ] ; then
		echo "Press any key to continue!"
		char=`get_char`
	fi
}

function pack_error()
{
	echo -e "\033[47;31mERROR: $*\033[0m"
}

function pack_warn()
{
	echo -e "\033[47;34mWARN: $*\033[0m"
}

function pack_info()
{
	echo -e "\033[47;30mINFO: $*\033[0m"
}

source scripts/shflags

# define option, format:
#   'long option' 'default value' 'help message' 'short option'
DEFINE_string 'chip' '' 'chip to build, e.g. sun7i' 'c'
DEFINE_string 'platform' '' 'platform to build, e.g. linux, android, camdroid' 'p'
DEFINE_string 'board' '' 'board to build, e.g. evb' 'b'
DEFINE_string 'kernel' '' 'kernel to build, e.g. linux-3.4, linux-3.10' 'k'
DEFINE_string 'debug_mode' 'uart0' 'config debug mode, e.g. uart0, card0' 'd'
DEFINE_string 'signture' 'none' 'pack boot signture to do secure boot' 's'
DEFINE_string 'secure' 'none' 'pack secure boot with -v arg' 'v'
DEFINE_string 'mode' 'normal' 'pack dump firmware' 'm'
DEFINE_string 'function' 'android' 'pack private firmware' 'f'
DEFINE_string 'topdir' 'none' 'sdk top dir' 't'

# parse the command-line
FLAGS "$@" || exit $?
eval set -- "${FLAGS_ARGV}"

PACK_CHIP=${FLAGS_chip}
PACK_PLATFORM=${FLAGS_platform}
PACK_BOARD=${FLAGS_board}
PACK_KERN=${FLAGS_kernel}
PACK_DEBUG=${FLAGS_debug_mode}
PACK_SIG=${FLAGS_signture}
PACK_SECURE=${FLAGS_secure}
PACK_MODE=${FLAGS_mode}
PACK_FUNC=${FLAGS_function}
PACK_TOPDIR=${FLAGS_topdir}
MULTI_CONFIG_INDEX=0

SUFFIX=""

ROOT_DIR=${PACK_TOPDIR}/out/${PACK_BOARD}
OTA_TEST_NAME="ota_test"
export PATH=${PACK_TOPDIR}/out/host/bin/:$PATH

if [ "x${PACK_CHIP}" = "xsun5i" ]; then
	PACK_BOARD_PLATFORM=unclear
	ARCH=arm
elif [ "x${PACK_CHIP}" = "xsun8iw5p1" ]; then
	PACK_BOARD_PLATFORM=astar
	ARCH=arm
elif [ "x${PACK_CHIP}" = "xsun8iw6p1" ]; then
	PACK_BOARD_PLATFORM=octopus
	ARCH=arm
elif [ "x${PACK_CHIP}" = "xsun8iw8p1" ]; then
	PACK_BOARD_PLATFORM=banjo
	ARCH=arm
elif [ "x${PACK_CHIP}" = "xsun8iw11p1" ]; then
	PACK_BOARD_PLATFORM=azalea
	ARCH=arm
elif [ "x${PACK_CHIP}" = "xsun8iw10p1" ]; then
	PACK_BOARD_PLATFORM=cello
	ARCH=arm
elif [ "x${PACK_CHIP}" = "xsun8iw15p1" ]; then
	PACK_BOARD_PLATFORM=mandolin
	ARCH=arm
#elif [ "x${PACK_CHIP}" = "xsun3iw1p1" ]; then
#	PACK_BOARD_PLATFORM=sitar
#	ARCH=arm
elif [ "x${PACK_CHIP}" = "xsun3iw1p1" ]; then
	PACK_BOARD_PLATFORM=${PACK_BOARD%%-*}
	ARCH=arm
elif [ "x${PACK_CHIP}" = "xsun50iw1p1" ]; then
	PACK_BOARD_PLATFORM=tulip
	ARCH=arm64
elif [ "x${PACK_CHIP}" = "xsun50iw3p1" ]; then
        PACK_BOARD_PLATFORM=koto
        ARCH=arm64
else
	echo "board_platform($PACK_CHIP) not support"
fi

tools_file_list=(
generic/tools/split_xxxx.fex
${PACK_BOARD_PLATFORM}-common/tools/split_xxxx.fex
generic/tools/usbtool_test.fex
${PACK_BOARD_PLATFORM}-common/tools/usbtool_test.fex
generic/tools/cardscript.fex
generic/tools/cardscript_secure.fex
${PACK_BOARD_PLATFORM}-common/tools/cardscript.fex
${PACK_BOARD_PLATFORM}-common/tools/cardscript_secure.fex
generic/tools/cardtool.fex
${PACK_BOARD_PLATFORM}-common/tools/cardtool.fex
generic/tools/usbtool.fex
${PACK_BOARD_PLATFORM}-common/tools/usbtool.fex
generic/tools/aultls32.fex
${PACK_BOARD_PLATFORM}-common/tools/aultls32.fex
generic/tools/aultools.fex
${PACK_BOARD_PLATFORM}-common/tools/aultools.fex
)

configs_file_list=(
generic/toc/toc1.fex
generic/toc/toc0.fex
generic/toc/boot_package.fex
generic/dtb/sunxi.fex
generic/configs/*.fex
generic/configs/*.cfg
${PACK_BOARD_PLATFORM}-common/configs/*.fex
${PACK_BOARD_PLATFORM}-common/configs/*.cfg
${PACK_BOARD}/configs/default/*.fex
${PACK_BOARD}/configs/default/*.cfg
${PACK_BOARD}/configs/${BOARD}/*.fex
${PACK_BOARD}/configs/${BOARD}/*.cfg
)

#BPI
boot_resource_list=(
generic/boot-resource/boot-resource:image/
generic/boot-resource/boot-resource.ini:image/
${PACK_BOARD_PLATFORM}-common/boot-resource:image/
${PACK_BOARD_PLATFORM}-common/boot-resource.ini:image/
${PACK_BOARD}/configs/default/*.bmp:image/boot-resource/
${PACK_BOARD}/configs/${BOARD}/*.bmp:image/boot-resource/
)

boot_file_list=(
${PACK_BOARD_PLATFORM}-common/bin/boot0_nand_${PACK_CHIP}.bin:image/boot0_nand.fex
${PACK_BOARD_PLATFORM}-common/bin/boot0_sdcard_${PACK_CHIP}.bin:image/boot0_sdcard.fex
${PACK_BOARD_PLATFORM}-common/bin/boot0_spinor_${PACK_CHIP}.bin:image/boot0_spinor.fex
${PACK_BOARD_PLATFORM}-common/bin/fes1_${PACK_CHIP}.bin:image/fes1.fex
${PACK_BOARD_PLATFORM}-common/bin/u-boot-${PACK_CHIP}.bin:image/u-boot.fex
${PACK_BOARD_PLATFORM}-common/bin/bl31.bin:image/monitor.fex
${PACK_BOARD_PLATFORM}-common/bin/scp.bin:image/scp.fex
${PACK_BOARD_PLATFORM}-common/bin/u-boot-spinor-${PACK_CHIP}.bin:image/u-boot-spinor.fex
${PACK_BOARD_PLATFORM}-common/bin/boot0_nand_${PACK_CHIP}-${OTA_TEST_NAME}.bin:image/boot0_nand-${OTA_TEST_NAME}.fex
${PACK_BOARD_PLATFORM}-common/bin/boot0_sdcard_${PACK_CHIP}-${OTA_TEST_NAME}.bin:image/boot0_sdcard-${OTA_TEST_NAME}.fex
${PACK_BOARD_PLATFORM}-common/bin/boot0_spinor_${PACK_CHIP}-${OTA_TEST_NAME}.bin:image/boot0_spinor-${OTA_TEST_NAME}.fex
${PACK_BOARD_PLATFORM}-common/bin/u-boot-${PACK_CHIP}-${OTA_TEST_NAME}.bin:image/u-boot-${OTA_TEST_NAME}.fex
${PACK_BOARD_PLATFORM}-common/bin/u-boot-spinor-${PACK_CHIP}-${OTA_TEST_NAME}.bin:image/u-boot-spinor-${OTA_TEST_NAME}.fex

${PACK_BOARD}/bin/boot0_nand_${PACK_CHIP}.bin:image/boot0_nand.fex
${PACK_BOARD}/bin/boot0_sdcard_${PACK_CHIP}.bin:image/boot0_sdcard.fex
${PACK_BOARD}/bin/boot0_spinor_${PACK_CHIP}.bin:image/boot0_spinor.fex
${PACK_BOARD}/bin/fes1_${PACK_CHIP}.bin:image/fes1.fex
${PACK_BOARD}/bin/u-boot-${PACK_CHIP}.bin:image/u-boot.fex
${PACK_BOARD}/bin/bl31.bin:image/monitor.fex
${PACK_BOARD}/bin/scp.bin:image/scp.fex
${PACK_BOARD}/bin/u-boot-spinor-${PACK_CHIP}.bin:image/u-boot-spinor.fex
${PACK_BOARD}/bin/boot0_nand_${PACK_CHIP}-${OTA_TEST_NAME}.bin:image/boot0_nand-${OTA_TEST_NAME}.fex
${PACK_BOARD}/bin/boot0_sdcard_${PACK_CHIP}-${OTA_TEST_NAME}.bin:image/boot0_sdcard-${OTA_TEST_NAME}.fex
${PACK_BOARD}/bin/boot0_spinor_${PACK_CHIP}-${OTA_TEST_NAME}.bin:image/boot0_spinor-${OTA_TEST_NAME}.fex
${PACK_BOARD}/bin/u-boot-${PACK_CHIP}-${OTA_TEST_NAME}.bin:image/u-boot-${OTA_TEST_NAME}.fex
${PACK_BOARD}/bin/u-boot-spinor-${PACK_CHIP}-${OTA_TEST_NAME}.bin:image/u-boot-spinor-${OTA_TEST_NAME}.fex
)

boot_file_secure=(
${PACK_BOARD_PLATFORM}-common/bin/semelis_${PACK_CHIP}.bin:image/semelis.bin
${PACK_BOARD_PLATFORM}-common/bin/optee_${PACK_CHIP}.bin:image/optee.bin
${PACK_BOARD_PLATFORM}-common/bin/sboot_${PACK_CHIP}.bin:image/sboot.bin
${PACK_BOARD_PLATFORM}-common/bin/sboot_${PACK_CHIP}-${OTA_TEST_NAME}.bin:image/sboot-${OTA_TEST_NAME}.bin

${PACK_BOARD}/bin/semelis_${PACK_CHIP}.bin:image/semelis.bin
${PACK_BOARD}/bin/optee_${PACK_CHIP}.bin:image/optee.bin
${PACK_BOARD}/bin/sboot_${PACK_CHIP}.bin:image/sboot.bin
${PACK_BOARD}/bin/sboot_${PACK_CHIP}-${OTA_TEST_NAME}.bin:image/sboot-${OTA_TEST_NAME}.bin
)

a64_boot_file_secure=(
${PACK_BOARD_PLATFORM}-common/bin/optee_${PACK_CHIP}.bin:image/optee.fex
${PACK_BOARD_PLATFORM}-common/bin/sboot_${PACK_CHIP}.bin:image/sboot.bin
# ${PACK_BOARD_PLATFORM}-common/bin/sboot_${PACK_CHIP}-${OTA_TEST_NAME}.bin:image/sboot-${OTA_TEST_NAME}.bin
)

function show_boards()
{
	printf "\nAll avaiable chips, platforms and boards:\n\n"
	printf "Chip            Board\n"
	for chipdir in $(find chips/ -mindepth 1 -maxdepth 1 -type d) ; do
		chip=`basename ${chipdir}`
		printf "${chip}\n"
		for boarddir in $(find chips/${chip}/configs/${platform} \
			-mindepth 1 -maxdepth 1 -type d) ; do
			board=`basename ${boarddir}`
			printf "                ${board}\n"
		done
	done
	printf "\nFor Usage:\n"
	printf "     $(basename $0) -h\n\n"
}

function uart_switch()
{
	rm -rf ${ROOT_DIR}/image/awk_debug_card0
	touch ${ROOT_DIR}/image/awk_debug_card0
	TX=`awk  '$0~"'$PACK_CHIP'"{print $2}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_pin`
	RX=`awk  '$0~"'$PACK_CHIP'"{print $3}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_pin`
	PORT=`awk  '$0~"'$PACK_CHIP'"{print $4}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_pin`
	MS=`awk  '$0~"'$PACK_CHIP'"{print $5}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_pin`
	CK=`awk  '$0~"'$PACK_CHIP'"{print $6}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_pin`
	DO=`awk  '$0~"'$PACK_CHIP'"{print $7}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_pin`
	DI=`awk  '$0~"'$PACK_CHIP'"{print $8}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_pin`

	BOOT_UART_ST=`awk  '$0~"'$PACK_CHIP'"{print $2}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	BOOT_PORT_ST=`awk  '$0~"'$PACK_CHIP'"{print $3}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	BOOT_TX_ST=`awk  '$0~"'$PACK_CHIP'"{print $4}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	BOOT_RX_ST=`awk  '$0~"'$PACK_CHIP'"{print $5}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	UART0_ST=`awk  '$0~"'$PACK_CHIP'"{print $6}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	UART0_USED_ST=`awk  '$0~"'$PACK_CHIP'"{print $7}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	UART0_PORT_ST=`awk  '$0~"'$PACK_CHIP'"{print $8}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	UART0_TX_ST=`awk  '$0~"'$PACK_CHIP'"{print $9}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	UART0_RX_ST=`awk  '$0~"'$PACK_CHIP'"{print $10}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	UART1_ST=`awk  '$0~"'$PACK_CHIP'"{print $11}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	JTAG_ST=`awk  '$0~"'$PACK_CHIP'"{print $12}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	MS_ST=`awk  '$0~"'$PACK_CHIP'"{print $13}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	CK_ST=`awk  '$0~"'$PACK_CHIP'"{print $14}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	DO_ST=`awk  '$0~"'$PACK_CHIP'"{print $15}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	DI_ST=`awk  '$0~"'$PACK_CHIP'"{print $16}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	MMC0_ST=`awk  '$0~"'$PACK_CHIP'"{print $17}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	MMC0_USED_ST=`awk  '$0~"'$PACK_CHIP'"{print $18}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`

	echo '$0!~";" && $0~"'$BOOT_TX_ST'"{if(C)$0="'$BOOT_TX_ST' = '$TX'"} \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '$0!~";" && $0~"'$BOOT_RX_ST'"{if(C)$0="'$BOOT_RX_ST' = '$RX'"} \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '$0!~";" && $0~"'$BOOT_PORT_ST'"{if(C)$0="'$BOOT_PORT_ST' = '$PORT'"} \' >> ${ROOT_DIR}/image/awk_debug_card0
	if [ "`grep "auto_print_used" "${ROOT_DIR}/image//sys_config${SUFFIX}.fex" | grep "1"`" ]; then
		echo '$0!~";" && $0~"'$MMC0_USED_ST'"{if(A)$0="'$MMC0_USED_ST' = 1";A=0} \' >> ${ROOT_DIR}/image/awk_debug_card0
	else
		echo '$0!~";" && $0~"'$MMC0_USED_ST'"{if(A)$0="'$MMC0_USED_ST' = 0";A=0} \' >> ${ROOT_DIR}/image/awk_debug_card0
	fi
	echo '$0!~";" && $0~"\\['$MMC0_ST'\\]"{A=1}  \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '$0!~";" && $0~"'$UART0_TX_ST'"{if(B)$0="'$UART0_TX_ST' = '$TX'"} \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '$0!~";" && $0~"'$UART0_RX_ST'"{if(B)$0="'$UART0_RX_ST' = '$RX'"} \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '$0!~";" && $0~"\\['$UART0_ST'\\]"{B=1} \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '$0!~";" && $0~"'$UART0_USED_ST'"{if(B)$0="'$UART0_USED_ST' = 1"}  \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '/^'$UART0_PORT_ST'/{next} \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '$0!~";" && $0~"\\['$UART1_ST'\\]"{B=0} \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '$0!~";" && $0~"\\['$BOOT_UART_ST'\\]"{C=1} \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '$0!~";" && $0~"\\['$JTAG_ST'\\]"{C=0} \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '$0!~";" && $0~"'$MS_ST'"{$0="'$MS_ST' = '$MS'"} \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '$0!~";" && $0~"'$CK_ST'"{$0="'$CK_ST' = '$CK'"} \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '$0!~";" && $0~"'$DO_ST'"{$0="'$DO_ST' = '$DO'"} \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '$0!~";" && $0~"'$DI_ST'"{$0="'$DI_ST' = '$DI'"} \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '1' >> ${ROOT_DIR}/image/awk_debug_card0

	if [ "`grep "auto_print_used" "${ROOT_DIR}/image/sys_config${SUFFIX}.fex" | grep "1"`" ]; then
		sed -i -e '/^uart0_rx/a\pinctrl-1=' ${ROOT_DIR}/image/sys_config${SUFFIX}.fex
		sed -i -e '/^uart0_rx/a\pinctrl-0=' ${ROOT_DIR}/image/sys_config${SUFFIX}.fex
	fi
	awk -f ${ROOT_DIR}/image/awk_debug_card0 ${ROOT_DIR}/image/sys_config${SUFFIX}.fex > ${ROOT_DIR}/image/sys_config${SUFFIX}_debug.fex
	rm -f ${ROOT_DIR}/image/sys_config${SUFFIX}.fex
	mv ${ROOT_DIR}/image/sys_config${SUFFIX}_debug.fex ${ROOT_DIR}/image/sys_config${SUFFIX}.fex
	echo "uart -> card0"
}

function copy_ota_test_file()
{
	printf "ota test bootloader by diff bootlogo\n"
	mv ${ROOT_DIR}/image/boot-resource/bootlogo_ota_test.bmp ${ROOT_DIR}/image/boot-resource/bootlogo.bmp

	printf "copying ota test boot file\n"
	if [ -f ${ROOT_DIR}/image/sys_partition_nor.fex -o \
	-f ${ROOT_DIR}/image/sys_partition_nor_${PACK_PLATFORM}.fex ];  then
		mv ${ROOT_DIR}/image/boot0_spinor-${OTA_TEST_NAME}.fex	${ROOT_DIR}/image/boot0_spinor.fex
		mv ${ROOT_DIR}/image/u-boot-spinor-${OTA_TEST_NAME}.fex	${ROOT_DIR}/image/u-boot-spinor.fex
	else
		mv ${ROOT_DIR}/image/boot0_nand-${OTA_TEST_NAME}.fex		${ROOT_DIR}/image/boot0_nand.fex
		mv ${ROOT_DIR}/image/boot0_sdcard-${OTA_TEST_NAME}.fex	${ROOT_DIR}/image/boot0_sdcard.fex
		mv ${ROOT_DIR}/image/u-boot-${OTA_TEST_NAME}.fex		${ROOT_DIR}/image/u-boot.fex
	fi

	if [ "x${PACK_SECURE}" = "xsecure" -o  "x${PACK_SIG}" = "prev_refurbish"] ; then
		printf "Copying ota test secure boot file\n"
		mv ${ROOT_DIR}/image/sboot-${OTA_TEST_NAME}.bin ${ROOT_DIR}/image/sboot.bin
	fi

	printf "OTA test env by bootdelay(10) and logolevel(8)\n"
	sed -i 's/\(logolevel=\).*/\18/' ${ROOT_DIR}/image/env.cfg
	sed -i 's/\(bootdelay=\).*/\110/' ${ROOT_DIR}/image/env.cfg
}



# pack user resources to a vfat filesystem
# To use this, please add a folder "user-resource" in configs to save files, and add a partition to sys_partition.fex/sys_partition_nor.fex like this:
# [partition]
#	name         = user-res
#	size         = 1024
#	downloadfile = "user-resource.fex"
#	user_type    = 0x8000
function make_user_res()
{
	printf "make user resource for : $1\n"
	local USER_RES_SYS_PARTITION=$1
	local USER_RES_PART_NAME=user-res
	local USER_RES_FILE=user-resource
	printf "handle partition ${USER_RES_PART_NAME}\n"
	local USER_RES_PART_DOWNLOAD_FILE=`awk 'BEGIN {FS="\n"; RS=""} /'${USER_RES_PART_NAME}'/{print $0}' ${USER_RES_SYS_PARTITION} | awk '{if($1 == "downloadfile"){print $3}}' | sed 's/"//g'`
	local USER_RES_PART_SIZE=`awk 'BEGIN {FS="\n"; RS=""} /'${USER_RES_PART_NAME}'/{print $0}' $1 | awk '$0~"size"{print $3/2}'`
	local USER_RES_FILE_PATH=${PACK_TOPDIR}/target/allwinner/${PACK_BOARD}/configs/${USER_RES_FILE}
	if [ x${USER_RES_PART_DOWNLOAD_FILE} != x'' -a  x${USER_RES_PART_SIZE} != x'' ];then
		rm -f ${ROOT_DIR}/image/user-resource.fex
		mkfs.vfat ${ROOT_DIR}/image/user-resource.fex -C ${USER_RES_PART_SIZE}
		if [ -d ${USER_RES_FILE_PATH} ];then
			USER_RES_FILE_SIZE=`du --summarize "${USER_RES_FILE_PATH}" | awk '{print $1}'`
			printf "file size: ${USER_RES_FILE_SIZE}\n"
			printf "partition size: ${USER_RES_PART_SIZE}\n"
			if [ ${USER_RES_PART_SIZE} -le ${USER_RES_FILE_SIZE} ]; then
				printf "file size is larger than partition size, please check your configuration\n"
				printf "please enlarge size of ${USER_RES_PART_NAME} in sys_partition or remove some files in $USER_RES_FILE_PATH\n"
				exit -1
			fi
			mcopy -s -v -i ${ROOT_DIR}/image/${USER_RES_PART_DOWNLOAD_FILE} ${USER_RES_FILE_PATH}/* ::
			if [ $? -ne 0 ]; then
				printf "mcopy file fail, exit\n"
				exit -1
			fi
		else
			printf "can not find ${USER_RES_FILE_PATH}, ignore it\n"
		fi
	else
		printf "no user resource partitions\n"
	fi
}

function update_suffix()
{
	if [ $MULTI_CONFIG_INDEX -gt 0 ]; then
		SUFFIX="-$MULTI_CONFIG_INDEX"
	else
		SUFFIX=""
	fi
}

function fetch_multiconfig()
{
	MULTI_CONFIG_INDEX=0
	update_suffix
	while [ -e ${ROOT_DIR}/image/sys_config${SUFFIX}.fex ];do
		echo sys_config${SUFFIX} exist
		let "MULTI_CONFIG_INDEX=MULTI_CONFIG_INDEX+1"
		update_suffix
	done
	let "MULTI_CONFIG_INDEX=MULTI_CONFIG_INDEX-1"
	echo "Multiconfig num:"$MULTI_CONFIG_INDEX
}


ENV_SUFFIX=

function do_early_prepare()
{
	# Cleanup
	rm -rf ${ROOT_DIR}/image
	mkdir -p ${ROOT_DIR}/image
	do_prepare
	fetch_multiconfig
}

function do_prepare()
{
	if [ -z "${PACK_CHIP}" -o -z "${PACK_PLATFORM}" -o -z "${PACK_BOARD}" ] ; then
		pack_error "Invalid parameters Chip: ${PACK_CHIP}, \
			Platform: ${PACK_PLATFORM}, Board: ${PACK_BOARD}"
		show_boards
		exit 1
	fi

	if [ ! -d ${PACK_TOPDIR}/target/allwinner/${PACK_BOARD}/configs ] ; then
		pack_error "Board's directory \
			\"${PACK_TOPDIR}/target/allwinner/${PACK_BOARD}/configs\" is not exist."
		show_boards
		exit 1
	fi
	#TODO:diff kernel version support
	if [ -z "${PACK_KERN}" ] ; then
		printf "No kernel param, parse it from ${PACK_BOARD_PLATFORM}\n"
		if [ "x${PACK_BOARD_PLATFORM}" = "xtulip" ]; then
			PACK_KERN="linux-4.4"
			ENV_SUFFIX=4.4
		elif [ "x${PACK_BOARD_PLATFORM}" = "xkoto" -o "x${PACK_BOARD_PLATFORM}" = "xmandolin" ]; then
			PACK_KERN="linux-4.9"
			ENV_SUFFIX=4.9
		elif [ "x${PACK_BOARD_PLATFORM}" = "xazalea" -o "x${PACK_BOARD_PLATFORM}" = "xsitar" -o "x${PACK_BOARD_PLATFORM}" = "xcello" -o "x${PACK_BOARD_PLATFORM}" = "xviolin" ]; then
			PACK_KERN="linux-3.10"
			ENV_SUFFIX=3.10
		else
			PACK_KERN="linux-3.4"
			ENV_SUFFIX=3.4
		fi
		if [ -z "${PACK_KERN}" ] ; then
			pack_error "Failed to parse kernel param from ${PACK_BOARD_PLATFORM}"
			exit 1
		fi
	fi


	printf "copying tools file\n"
	for file in ${tools_file_list[@]} ; do
		cp -f ${PACK_TOPDIR}/target/allwinner/$file ${ROOT_DIR}/image/ 2> /dev/null
	done

	if [ "x${PACK_KERN}" = "xlinux-3.4" ]; then
		cp -f generic/tools/cardscript.fex ${ROOT_DIR}/image/ 2> /dev/null
	fi

	printf "copying configs file\n"
	for file in ${configs_file_list[@]} ; do
		cp -f ${PACK_TOPDIR}/target/allwinner/$file ${ROOT_DIR}/image/ 2> /dev/null
	done

	if [ -f ${ROOT_DIR}/image/sys_config_${PACK_KERN}${SUFFIX}.fex ]; then
		cp ${ROOT_DIR}/image/sys_config_${PACK_KERN}${SUFFIX}.fex ${ROOT_DIR}/image/sys_config${SUFFIX}.fex
	fi

	# get storage_type value from sys_config.fex
	if [ ! -f ${ROOT_DIR}/image/sys_config${SUFFIX}.fex ];then
		echo "sys_config${SUFFIX}.fex is not exist."
		exit 1
	fi

	local storage_type
	storage_type=`sed -e '/^$/d' -e '/^;/d' -e '/^\[/d' -n -e '/^storage_type/p' ${ROOT_DIR}/image/sys_config${SUFFIX}.fex | sed 's/=/ /g' | awk '{ print $2;}'`
	echo "storage_type value is ${storage_type}"

	case ${storage_type} in
		3)
		echo "storage type is nor"
		if [ -f ${ROOT_DIR}/image/image_nor.cfg ];then
			echo "image_nor.cfg is exist"
			mv ${ROOT_DIR}/image/image_nor.cfg ${ROOT_DIR}/image/image.cfg && echo "mv image_nor.cfg image.cfg"
		fi
		;;
		-1)
		;;
		*)
		if [ -f ${ROOT_DIR}/image/sys_partition_nor.fex ];then
			rm ${ROOT_DIR}/image/sys_partition_nor.fex && echo "rm ${ROOT_DIR}/image/sys_partition_nor.fex"
		fi
		if [ -f ${ROOT_DIR}/image/image_nor.cfg ];then
			rm ${ROOT_DIR}/image/image_nor.cfg && echo "rm ${ROOT_DIR}/image/image_nor.cfg"
		fi
		;;
	esac

	# amend env copy
	mv ${ROOT_DIR}/image/env-${ENV_SUFFIX}.cfg ${ROOT_DIR}/image/env.cfg 2> /dev/null
	#BPI
	cp -f ${PACK_TOPDIR}/target/allwinner/${PACK_BOARD}/configs/${BOARD}/env.cfg ${ROOT_DIR}/image/env.cfg 2> /dev/null
	# If platform config files exist, we will cover the default files
	# For example, mv out/image_linux.cfg out/image.cfg
	cd ${ROOT_DIR}
	find image/* -type f -a \( -name "*.fex" -o -name "*.cfg" \) -print | \
		sed "s#\(.*\)_${PACK_PLATFORM}\(\..*\)#mv -fv & \1\2#e"
	cd -

	if [ "x${PACK_MODE}" = "xdump" ] ; then
		cp -vf ${ROOT_DIR}/image/sys_partition_dump.fex ${ROOT_DIR}/image/sys_partition.fex
		cp -vf ${ROOT_DIR}/image/usbtool_test.fex ${ROOT_DIR}/image/usbtool.fex
	elif [ "x${PACK_FUNC}" = "xprvt" ] ; then
		cp -vf ${ROOT_DIR}/image/sys_partition_private.fex ${ROOT_DIR}/image/sys_partition.fex
	fi

	grep "CONFIG_USE_DM_VERITY=y" ${PACK_TOPDIR}/.config > /dev/null
	if [ $? -eq 0 ]; then
		cp -vf ${ROOT_DIR}/image/sys_partition_verity.fex ${ROOT_DIR}/image/sys_partition.fex
	fi

	printf "copying boot resource\n"
	for file in ${boot_resource_list[@]} ; do
		cp -rf ${PACK_TOPDIR}/target/allwinner/`echo $file | awk -F: '{print $1}'` \
			${ROOT_DIR}/`echo $file | awk -F: '{print $2}'` 2>/dev/null
	done
	lzma e ${ROOT_DIR}/image/boot-resource/bootlogo.bmp ${ROOT_DIR}/image/bootlogo.bmp.lzma
	printf "copying boot file\n"
	for file in ${boot_file_list[@]} ; do
		cp -f ${PACK_TOPDIR}/target/allwinner/`echo $file | awk -F: '{print $1}'` \
			${ROOT_DIR}/`echo $file | awk -F: '{print $2}'` 2>/dev/null
	done

	if [ "x${PACK_MODE}" = "xcrashdump" ] ; then
		cp -vf ${ROOT_DIR}/image/sys_partition_dump.fex ${ROOT_DIR}/image/sys_partition.fex
		cp -vf ${ROOT_DIR}/image/image_crashdump.cfg ${ROOT_DIR}/image/image.cfg
		cp -vf ${PACK_BOARD_PLATFORM}-common/bin/u-boot-crashdump-${PACK_CHIP}.bin ${ROOT_DIR}/image/u-boot.fex
	fi

	if [ "x${ARCH}" != "xarm64" ] ; then
		if [ "x${PACK_SECURE}" = "xsecure" -o "x${PACK_SIG}" = "xsecure" -o  "x${PACK_SIG}" = "xprev_refurbish" ] ; then
			printf "copying secure boot file\n"
			for file in ${boot_file_secure[@]} ; do
				cp -f ${PACK_TOPDIR}/target/allwinner/`echo $file | awk -F: '{print $1}'` \
					${ROOT_DIR}/`echo $file | awk -F: '{print $2}'`
			done
		fi
	else
		if [ "x${PACK_SECURE}" = "xsecure" -o "x${PACK_SIG}" = "xsecure" -o  "x${PACK_SIG}" = "xprev_refurbish" ] ; then
			printf "copying arm64 secure boot file\n"
			for file in ${a64_boot_file_secure[@]} ; do
				cp -f ${PACK_TOPDIR}/target/allwinner/`echo $file | awk -F: '{print $1}'` \
					${ROOT_DIR}/`echo $file | awk -F: '{print $2}'`
			done
		fi
	fi

	# If platform config use
	if [ -f ${PACK_TOPDIR}/target/allwinner/${PACK_BOARD_PLATFORM}-common/tools/plat_config.sh ] ; then
		${PACK_TOPDIR}/target/allwinner/${PACK_BOARD_PLATFORM}-common/tools/plat_config.sh
	fi


	if [ "x${PACK_SECURE}" = "xsecure"  -o "x${PACK_SIG}" = "xsecure" ] ; then
		printf "add burn_secure_mode in target in sys config\n"
		sed -i -e '/^\[target\]/a\burn_secure_mode=1' ${ROOT_DIR}/image/sys_config${SUFFIX}.fex
		sed -i -e '/^\[platform\]/a\secure_without_OS=0' ${ROOT_DIR}/image/sys_config${SUFFIX}.fex
	elif [ "x${PACK_SIG}" = "xprev_refurbish" ] ; then
		printf "add burn_secure_mode in target in sys config\n"
		sed -i -e '/^\[target\]/a\burn_secure_mode=1' ${ROOT_DIR}/image/sys_config${SUFFIX}.fex
		sed -i -e '/^\[platform\]/a\secure_without_OS=1' ${ROOT_DIR}/image/sys_config${SUFFIX}.fex
	else
		sed -i '/^burn_secure_mod/d' ${ROOT_DIR}/image/sys_config${SUFFIX}.fex
		sed -i '/^secure_without_OS/d' ${ROOT_DIR}/image/sys_config${SUFFIX}.fex
	fi

	if [ "x${PACK_MODE}" = "xota_test" ] ; then
		printf "copy ota test file\n"
		copy_ota_test_file
	fi

	# Here, we can switch uart to card or normal
	if [ "x${PACK_DEBUG}" = "xcard0" -a "x${PACK_MODE}" != "xdump" \
		-a "x${PACK_FUNC}" != "xprvt" ] ; then \
		uart_switch
	fi

	sed -i 's/\\boot-resource/\/boot-resource/g' ${ROOT_DIR}/image/boot-resource.ini
	sed -i 's/\\\\/\//g' ${ROOT_DIR}/image/image.cfg
	sed -i 's/^imagename/;imagename/g' ${ROOT_DIR}/image/image.cfg

	IMG_NAME="${PACK_PLATFORM}_${PACK_BOARD}_${PACK_DEBUG}"

	if [ "x${PACK_SIG}" != "xnone" ]; then
		IMG_NAME="${IMG_NAME}_${PACK_SIG}"
	fi

	if [ "x${PACK_MODE}" = "xdump" -o "x${PACK_MODE}" = "xota_test" -o "x${PACK_MODE}" = "xcrashdump" ] ; then
		IMG_NAME="${IMG_NAME}_${PACK_MODE}"
	fi

	if [ "x${PACK_FUNC}" = "xprvt" ] ; then
		IMG_NAME="${IMG_NAME}_${PACK_FUNC}"
	fi

	if [ "x${PACK_SECURE}" = "xsecure" ] ; then
		IMG_NAME="${IMG_NAME}_${PACK_SECURE}"
	fi

	if [ "x${PACK_FUNC}" = "xprev_refurbish" ] ; then
		IMG_NAME="${IMG_NAME}_${PACK_FUNC}"
	fi

	if [ "x${PACK_SECURE}" != "xnone" -o "x${PACK_SIG}" != "xnone" ]; then
		MAIN_VERION=`awk  '$0~"MAIN_VERSION"{printf"%d", $3}' ${PACK_TOPDIR}/target/allwinner/generic/version/version_base.mk`
		SUB_VERION=`awk  '$0~"SUB_VERSION"{printf"%d", $3}' ${PACK_TOPDIR}/target/allwinner/generic/version/version_base.mk`

		IMG_NAME="${IMG_NAME}_v${MAIN_VERION}-${SUB_VERION}.img"
	else
		IMG_NAME="${IMG_NAME}.img"
	fi

	echo "imagename = $IMG_NAME" >> ${ROOT_DIR}/image/image.cfg
	echo "" >> ${ROOT_DIR}/image/image.cfg

	# boot time optimization:
	# 1.remove uboot uart log;
	# 2.do not check kernel image crc.
	# 3.remove kernel uart log.
	# 4.set rootfstype.
	grep "CONFIG_BOOT_TIME_OPTIMIZATION=y" ${PACK_TOPDIR}/.config > /dev/null
	if [ $? -eq 0 ]; then
		#sed -i "/debug_mode/d" ${ROOT_DIR}/image/sys_config${SUFFIX}.fex && sed -i '/^\[platform\]$/a\debug_mode\ \ =\ 0' ${ROOT_DIR}/image/sys_config${SUFFIX}.fex
		sed -i "/^verify=/d" ${ROOT_DIR}/image/env.cfg && sed -i '/^init=/a\verify=no' ${ROOT_DIR}/image/env.cfg
		#sed -i "/^loglevel=/d" ${ROOT_DIR}/image/env.cfg && sed -i '/^init=/a\loglevel=0' ${ROOT_DIR}/image/env.cfg

		grep "CONFIG_TARGET_ROOTFS_SQUASHFS=y" ${PACK_TOPDIR}/.config > /dev/null
		if [ $? -eq 0 ]; then
			rootfstype=squashfs
		fi

                grep "CONFIG_TARGET_ROOTFS_EXT4FS=y" ${PACK_TOPDIR}/.config > /dev/null
                if [ $? -eq 0 ]; then
                        rootfstype=ext4
                fi

		if [ "x${rootfstype}" != "x" ]; then
			sed -i "s/setargs_.*=.*/& rootfstype=${rootfstype}/" ${ROOT_DIR}/image/env.cfg
		fi
	fi

	# for busybox init, default use /pseudo_init as init process.
	grep "CONFIG_SYSTEM_INIT_BUSYBOX=y" ${PACK_TOPDIR}/.config > /dev/null
	if [ $? -eq 0 ]; then
		sed -i "/^init=/d" ${ROOT_DIR}/image/env.cfg && sed -i '/^mmc_root=/a\init=\/pseudo_init' ${ROOT_DIR}/image/env.cfg
	fi

	if [ -e ${ROOT_DIR}/image/sys_partition_nor.fex ];then
		make_user_res ${ROOT_DIR}/image/sys_partition_nor.fex
	else
		make_user_res ${ROOT_DIR}/image/sys_partition.fex
	fi

}

function do_ini_to_dts()
{
	if [ "x${PACK_KERN}" == "xlinux-3.4" ]; then
		return
	fi
	if [ "x${PACK_KERN}" == "xlinux-4.4" -a "x${ARCH}" == "xarm64" ]; then
		local DTC_DEP_FILE=${PACK_TOPDIR}/lichee/$PACK_KERN/arch/$ARCH/boot/dts/sunxi/.${PACK_CHIP}-${PACK_BOARD}${SUFFIX}.dtb.d.dtc.tmp
		local DTC_SRC_PATH=${PACK_TOPDIR}/lichee/$PACK_KERN/arch/$ARCH/boot/dts/sunxi/
		local DTC_SRC_FILE=${PACK_TOPDIR}/lichee/$PACK_KERN/arch/$ARCH/boot/dts/sunxi/.${PACK_CHIP}-${PACK_BOARD}${SUFFIX}.dtb.dts.tmp
	elif [ "x${PACK_KERN}" == "xlinux-4.9" -a "x${ARCH}" == "xarm64" ]; then
		local DTC_DEP_FILE=${PACK_TOPDIR}/lichee/$PACK_KERN/arch/$ARCH/boot/dts/sunxi/.${PACK_CHIP}-${PACK_BOARD}${SUFFIX}.dtb.d.dtc.tmp
		local DTC_SRC_PATH=${PACK_TOPDIR}/lichee/$PACK_KERN/arch/$ARCH/boot/dts/sunxi/
		local DTC_SRC_FILE=${PACK_TOPDIR}/lichee/$PACK_KERN/arch/$ARCH/boot/dts/sunxi/.${PACK_CHIP}-${PACK_BOARD}${SUFFIX}.dtb.dts.tmp
	else
		local DTC_DEP_FILE=${PACK_TOPDIR}/lichee/$PACK_KERN/arch/$ARCH/boot/dts/.${PACK_CHIP}-${PACK_BOARD}${SUFFIX}.dtb.d.dtc.tmp
		local DTC_SRC_PATH=${PACK_TOPDIR}/lichee/$PACK_KERN/arch/$ARCH/boot/dts/
		local DTC_SRC_FILE=${PACK_TOPDIR}/lichee/$PACK_KERN/arch/$ARCH/boot/dts/.${PACK_CHIP}-${PACK_BOARD}${SUFFIX}.dtb.dts.tmp
	fi
	local DTC_COMPILER=${PACK_TOPDIR}/lichee/$PACK_KERN/scripts/dtc/dtc
	local DTC_INI_FILE_BASE=${ROOT_DIR}/image/sys_config${SUFFIX}.fex
	local DTC_INI_FILE=${ROOT_DIR}/image/sys_config${SUFFIX}_fix.fex
	local DTC_FLAGS=""

	cp $DTC_INI_FILE_BASE $DTC_INI_FILE
	sed -i "s/\(\[dram\)_para\(\]\)/\1\2/g" $DTC_INI_FILE
	sed -i "s/\(\[nand[0-9]\)_para\(\]\)/\1\2/g" $DTC_INI_FILE

	if [ ! -f $DTC_COMPILER ]; then
		pack_error "Script_to_dts: Can not find dtc compiler.\n"
		exit 1
	fi
	if [ ! -f $DTC_DEP_FILE ]; then
		printf "Script_to_dts: Can not find [%s-%s.dts]. Will use common dts file instead.\n" ${PACK_CHIP} ${PACK_BOARD}
		if [ "x${PACK_BOARD_PLATFORM}" = "xsitar" ] ; then
			DTC_DEP_FILE=${PACK_TOPDIR}/lichee/$PACK_KERN/arch/$ARCH/boot/dts/.${PACK_CHIP}-r6-soc${SUFFIX}.dtb.d.dtc.tmp
			DTC_SRC_FILE=${PACK_TOPDIR}/lichee/$PACK_KERN/arch/$ARCH/boot/dts/.${PACK_CHIP}-r6-soc${SUFFIX}.dtb.dts.tmp
		elif [ "x${PACK_BOARD_PLATFORM}" = "xviolin" ] ; then
				DTC_DEP_FILE=${PACK_TOPDIR}/lichee/$PACK_KERN/arch/$ARCH/boot/dts/.${PACK_CHIP}-violin-F1C200s${SUFFIX}.dtb.d.dtc.tmp
				DTC_SRC_FILE=${PACK_TOPDIR}/lichee/$PACK_KERN/arch/$ARCH/boot/dts/.${PACK_CHIP}-violin-F1C200s${SUFFIX}.dtb.dts.tmp
		else
			if [ "x${PACK_KERN}" == "xlinux-4.4" -a "x${ARCH}" == "xarm64" ]; then
				DTC_DEP_FILE=${PACK_TOPDIR}/lichee/$PACK_KERN/arch/$ARCH/boot/dts/sunxi/.${PACK_CHIP}-soc${SUFFIX}.dtb.d.dtc.tmp
				DTC_SRC_FILE=${PACK_TOPDIR}/lichee/$PACK_KERN/arch/$ARCH/boot/dts/sunxi/.${PACK_CHIP}-soc${SUFFIX}.dtb.dts.tmp
			elif [ "x${PACK_KERN}" == "xlinux-4.9" -a "x${ARCH}" == "xarm64" ]; then
				DTC_DEP_FILE=${PACK_TOPDIR}/lichee/$PACK_KERN/arch/$ARCH/boot/dts/sunxi/.${PACK_CHIP}-soc${SUFFIX}.dtb.d.dtc.tmp
				DTC_SRC_FILE=${PACK_TOPDIR}/lichee/$PACK_KERN/arch/$ARCH/boot/dts/sunxi/.${PACK_CHIP}-soc${SUFFIX}.dtb.dts.tmp
			else
				DTC_DEP_FILE=${PACK_TOPDIR}/lichee/$PACK_KERN/arch/$ARCH/boot/dts/.${PACK_CHIP}-soc${SUFFIX}.dtb.d.dtc.tmp
				DTC_SRC_FILE=${PACK_TOPDIR}/lichee/$PACK_KERN/arch/$ARCH/boot/dts/.${PACK_CHIP}-soc${SUFFIX}.dtb.dts.tmp
			fi
		fi
		#Disbale noisy checks
		if [ "x${PACK_KERN}" == "xlinux-4.9" ]; then
			DTC_FLAGS="-W no-unit_address_vs_reg"
		fi
	fi
	$DTC_COMPILER ${DTC_FLAGS} -O dtb -o ${ROOT_DIR}/image/sunxi${SUFFIX}.dtb	\
		-b 0			\
		-i $DTC_SRC_PATH	\
		-F $DTC_INI_FILE	\
		-d $DTC_DEP_FILE $DTC_SRC_FILE

	if [ $? -ne 0 ]; then
		pack_error "Conver script to dts failed"
		exit 1
	fi

	printf "Conver script to dts ok.\n"

	# It'is used for debug dtb
	$DTC_COMPILER  ${DTC_FLAGS} -I dtb -O dts -o ${ROOT_DIR}/image/.sunxi${SUFFIX}.dts ${ROOT_DIR}/image/sunxi${SUFFIX}.dtb

	return
}

function do_common()
{
	cd ${ROOT_DIR}/image

	if [ ! -f board_config.fex ]; then
		echo "[empty]" > board_config.fex
	fi

	busybox unix2dos sys_config${SUFFIX}.fex
	busybox unix2dos board_config.fex
	script  sys_config${SUFFIX}.fex > /dev/null
	cp -f   sys_config${SUFFIX}.bin config${SUFFIX}.fex
	script  board_config.fex > /dev/null
	cp -f board_config.bin board.fex

	busybox unix2dos sys_partition.fex
	script  sys_partition.fex > /dev/null

	# Those files for SpiNor. We will try to find sys_partition_nor.fex
	if [ -f sys_partition_nor.fex ];  then

		if [ -f "sunxi${SUFFIX}.dtb" ]; then
			cp sunxi${SUFFIX}.dtb sunxi${SUFFIX}.fex
		fi

		if [ -f "scp.fex" ]; then
			echo "update scp"
			update_scp scp.fex sunxi${SUFFIX}.fex >/dev/null
		fi
		# Here, will create sys_partition_nor.bin
		busybox unix2dos sys_partition_nor.fex
		script  sys_partition_nor.fex > /dev/null
		update_boot0 boot0_spinor.fex   sys_config${SUFFIX}.bin SDMMC_CARD > /dev/null
		if [ "x${PACK_KERN}" = "xlinux-3.4" ] ; then
			update_uboot -merge u-boot-spinor.fex  sys_config${SUFFIX}.bin > /dev/null
		else
			update_uboot -no_merge u-boot-spinor.fex  sys_config${SUFFIX}.bin > /dev/null
		fi

		if [ -f boot_package_nor.cfg -a	x${SUFFIX} == x'' ]; then
			echo "pack boot package"
			busybox unix2dos boot_package.cfg
			dragonsecboot -pack boot_package_nor.cfg
			cp boot_package.fex boot_package_nor.fex
		fi
		# Ugly, but I don't have a better way to change it.
		# We just set env's downloadfile name to env_nor.cfg in sys_partition_nor.fex
		# And if env_nor.cfg is not exist, we should copy one.
		if [ ! -f env_nor.cfg ]; then
			cp -f env.cfg env_nor.cfg >/dev/null 2<&1
		fi

		# Fixup boot mode for SPINor, just can bootm
		sed -i '/^boot_normal/s#\<boota\>#bootm#g' env_nor.cfg

		u_boot_env_gen env_nor.cfg env_nor.fex >/dev/null
	fi


	if [ -f "sunxi${SUFFIX}.dtb" ]; then
		cp sunxi${SUFFIX}.dtb sunxi${SUFFIX}.fex
		update_dtb sunxi${SUFFIX}.fex 4096
	fi

	if [ -f "scp.fex" ]; then
		echo "update scp"
		update_scp scp.fex sunxi${SUFFIX}.fex >/dev/null
	fi
	# Those files for Nand or Card
	update_boot0 boot0_nand.fex	sys_config${SUFFIX}.bin NAND > /dev/null
	update_boot0 boot0_sdcard.fex	sys_config${SUFFIX}.bin SDMMC_CARD > /dev/null
	if [ "x${PACK_KERN}" = "xlinux-3.4" ] ; then
		update_uboot -merge u-boot.fex sys_config${SUFFIX}.bin > /dev/null
	else
		update_uboot -no_merge u-boot.fex sys_config${SUFFIX}.bin > /dev/null
	fi
	update_fes1  fes1.fex           sys_config${SUFFIX}.bin > /dev/null
	fsbuild	     boot-resource.ini  split_xxxx.fex > /dev/null

	if [ -f boot_package.cfg  -a x${SUFFIX} == x'' ]; then
			pause "before boot package"
			echo "pack boot package"
			busybox unix2dos boot_package.cfg
			dragonsecboot -pack boot_package.cfg
			if [ $? -ne 0 ]
			then
				pack_error "dragon pack run error"
				exit 1
			fi

			update_toc1  boot_package.fex           sys_config${SUFFIX}.bin
			if [ $? -ne 0 ]
			then
				pack_error "update toc1 run error"
				exit 1
			fi
	fi

	if [ "x${PACK_FUNC}" = "xprvt" ] ; then
		u_boot_env_gen env_burn.cfg env.fex > /dev/null
	else
		u_boot_env_gen env.cfg env.fex > /dev/null
	fi

	if [ -f "arisc" ]; then
		ln -s arisc arisc.fex
	fi
}

function do_finish()
{
	# Yeah, it should contain all files into full_img.fex for spinor
	# Because, as usually, spinor image size is very small.
	# If fail to create full_img.fex, we should fake it empty.

	# WTF, it is so ugly!!! It must be sunxi_mbr.fex & sys_partition.bin,
	# not sunxi_mbr_xxx.fex & sys_partition_xxx.bin. In order to advoid this
	# loathsome thing, we need to backup & copy files. Check whether
	# sys_partition_nor.bin is exist, and create sunxi_mbr.fex for Nor.
	if [ -f sys_partition_nor.bin ]; then
		update_mbr sys_partition_nor.bin 1 sunxi_mbr_nor.fex
		if [ $? -ne 0 ]; then
			pack_error "update_mbr failed"
			exit 1
		fi
		#only uboot2011&linux-3.4 bsp used full img
		echo ".....11....."
		if [ "x${PACK_KERN}" = "xlinux-3.4" ] ; then
			echo "....2222....."
			BOOT1_FILE=u-boot-spinor.fex
			LOGIC_START=496 #496+16=512K
			merge_full_img --out full_img.fex \
				--boot0 boot0_spinor.fex \
				--boot1 ${BOOT1_FILE} \
				--mbr sunxi_mbr_nor.fex \
				--logic_start ${LOGIC_START} \
				--partition sys_partition_nor.bin
			if [ $? -ne 0 ]; then
				pack_error "merge_full_img failed"
				exit 1
			fi
			rm -f sys_partition_for_dragon.fex
		else
			echo "..............2222211111......."
			cp sys_partition_nor.fex sys_partition_for_dragon.fex
		fi
		cp sys_partition_nor.fex sys_partition.fex

	else
		echo ".....33333....."

		if [ "x${PACK_KERN}" = "xlinux-3.4" -a ! -f full_img.fex ] ; then
			echo ".....333>>>>>>....."
			echo ".....333>>>>>>>....."
			echo "full_img.fex is empty" > full_img.fex
		fi
		echo ".....44444....."
		update_mbr sys_partition.bin 4
		if [ $? -ne 0 ]; then
			pack_error "update_mbr failed"
			exit 1
		fi
		cp sys_partition.fex sys_partition_for_dragon.fex
	fi

	echo "....5555....."
	if [ -f sys_partition_for_dragon.fex ]; then
		echo "....5551111111....."
		do_dragon image.cfg sys_partition_for_dragon.fex
	else
		echo ".....6666....."
		do_dragon image.cfg
	fi

	cd ..
	printf "pack finish\n"
}

function do_dragon()
{
	pause "before dragon"
	dragon $@
        if [ $? -eq 0 ]; then
	    if [ -e ${IMG_NAME} ]; then
		    mv ${IMG_NAME} ../${IMG_NAME}
		    echo '----------image is at----------'
		    echo -e '\033[0;31;1m'
		    echo ${ROOT_DIR}/${IMG_NAME}
		    echo -e '\033[0m'
	    fi
        fi
}

function do_signature()
{
	# merge flag: '1' - merge atf/scp/uboot/optee in one package, '0' - do not merge.
	local merge_flag=0

	printf "prepare for signature by openssl\n"
	if [ "x${PACK_SIG}" = "xprev_refurbish" ] ; then
		if [ "x${ARCH}" = "xarm64" ] ; then
			cp -v ${PACK_TOPDIR}/target/allwinner/generic/sign_config/dragon_toc_a64_no_secureos.cfg dragon_toc.cfg
		else
			cp -v ${PACK_TOPDIR}/target/allwinner/generic/sign_config/dragon_toc_no_secureos.cfg dragon_toc.cfg
		fi
	else
		if [ "x${ARCH}" = "xarm64" ] ; then
			if [ -f ${PACK_TOPDIR}/target/allwinner/${PACK_BOARD_PLATFORM}-common/sign_config/dragon_toc_a64_package.cfg ] ; then
				cp -v ${PACK_TOPDIR}/target/allwinner/${PACK_BOARD_PLATFORM}-common/sign_config/dragon_toc_a64_package.cfg dragon_toc.cfg
				merge_flag=1
			elif [ -f ${PACK_TOPDIR}/target/allwinner/${PACK_BOARD_PLATFORM}-common/sign_config/dragon_toc_a64.cfg ] ; then
				cp -v ${PACK_TOPDIR}/target/allwinner/${PACK_BOARD_PLATFORM}-common/sign_config/dragon_toc_a64.cfg dragon_toc.cfg
			else
				cp -v ${PACK_TOPDIR}/target/allwinner/generic/sign_config/dragon_toc_a64.cfg dragon_toc.cfg
			fi
		else
			if [ -f ${PACK_TOPDIR}/target/allwinner/${PACK_BOARD_PLATFORM}-common/sign_config/dragon_toc.cfg ] ; then
				cp -v ${PACK_TOPDIR}/target/allwinner/${PACK_BOARD_PLATFORM}-common/sign_config/dragon_toc.cfg dragon_toc.cfg
			else
				cp -v ${PACK_TOPDIR}/target/allwinner/generic/sign_config/dragon_toc.cfg dragon_toc.cfg
			fi
		fi
	fi

	if [ $? -ne 0 ]
	then
		pack_error "dragon toc config file is not exist"
		exit 1
	fi

	rm -f cardscript.fex
	mv cardscript_secure.fex cardscript.fex
	if [ $? -ne 0 ]
	then
		pack_error "dragon cardscript_secure.fex file is not exist"
		exit 1
	fi

	if [ x${SUFFIX} == x'' ]; then
		dragonsecboot -toc0 dragon_toc.cfg ${ROOT_DIR}/keys  > /dev/null
		if [ $? -ne 0 ]
		then
			pack_error "dragon toc0 run error"
			exit 1
		fi
	fi

	update_toc0  toc0.fex           sys_config${SUFFIX}.bin
	if [ $? -ne 0 ]
	then
		pack_error "update toc0 run error"
		exit 1
	fi

	if [ x${SUFFIX} == x'' ]; then
		if [ ${merge_flag} == 1 ]; then
			printf "dragon boot package\n"
			dragonsecboot -pack dragon_toc.cfg
			if [ $? -ne 0 ]
			then
				pack_error "dragon boot_package run error"
				exit 1
			fi
		fi

		dragonsecboot -toc1 dragon_toc.cfg ${ROOT_DIR}/keys \
			${PACK_TOPDIR}/target/allwinner/generic/sign_config/cnf_base.cnf \
			${PACK_TOPDIR}/target/allwinner/generic/version/version_base.mk
		if [ $? -ne 0 ]
		then
			pack_error "dragon toc1 run error"
			exit 1
		fi

		sigbootimg --image boot.fex --cert toc1/cert/boot.der --output boot_sig.fex
		if [ $? -ne 0 ] ; then
			pack_error "Pack cert to image error"
			exit 1
		else
			mv -f boot_sig.fex boot.fex
		fi

	fi

	update_toc1  toc1.fex           sys_config${SUFFIX}.bin
	if [ $? -ne 0 ]
	then
		pack_error "update toc1 run error"
		exit 1
	fi
	echo "secure signature ok!"
}

function do_pack_tina()
{
	printf "packing for tina linux\n"

	rm -rf vmlinux.fex
	rm -rf boot.fex
	rm -rf rootfs.fex
	rm -rf kernel.fex
	rm -rf rootfs_squashfs.fex
	rm -rf usr.fex
	rm -rf recovery.fex
	#ln -s ${ROOT_DIR}/vmlinux.tar.bz2 vmlinux.fex

	if [ -f ${ROOT_DIR}/boot_initramfs.img ]; then
		ln -s ${ROOT_DIR}/boot_initramfs.img        boot.fex
	else
		ln -s ${ROOT_DIR}/boot.img        boot.fex
	fi

	ln -s ${ROOT_DIR}/rootfs.img     rootfs.fex
	if [ -f ${ROOT_DIR}/usr.img ]; then
		ln -s ${ROOT_DIR}/usr.img    usr.fex
	fi

	if [ -f ${ROOT_DIR}/boot_initramfs_recovery.img ]; then
		ln -s ${ROOT_DIR}/boot_initramfs_recovery.img recovery.fex
	else
		touch recovery.fex
		echo "recovery part not used!" > recovery.fex
	fi
	# Those files is ready for SPINor.
	#ln -s ${ROOT_DIR}/uImage          kernel.fex
	#ln -s ${ROOT_DIR}/rootfs.squashfs rootfs_squashfs.fex

	# add for dm-verity block
	grep "CONFIG_USE_DM_VERITY=y" ${PACK_TOPDIR}/.config > /dev/null
	if [ $? -eq 0 ]; then
		${PACK_TOPDIR}/scripts/dm-verity-block.sh ${ROOT_DIR}/rootfs.img ${ROOT_DIR}/verity/verity_block

		if [ -f ${ROOT_DIR}/verity/verity_block ]; then
			ln -s ${ROOT_DIR}/verity/verity_block verity_block.fex
		fi
	fi

	if [ "x${PACK_SIG}" = "xsecure" ] ; then
		echo "secure"
		do_signature
	elif [ "x${PACK_SIG}" = "xprev_refurbish" ] ; then
		echo "prev_refurbish"
		do_signature
	else
		echo "normal"
	fi
}


do_early_prepare

while [ $MULTI_CONFIG_INDEX -ge 0 ]; do
	update_suffix
	do_prepare
	do_ini_to_dts
	do_common
	let "MULTI_CONFIG_INDEX=MULTI_CONFIG_INDEX-1"
done

do_pack_${PACK_PLATFORM}
do_finish

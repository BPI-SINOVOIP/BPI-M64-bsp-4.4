#!/bin/bash

die() {
        echo "$*" >&2
        exit 1
}

[ -s "./env.sh" ] || die "please run ./configure first."

set -e

. ./env.sh
export MACH=sun50iw1p1

PACK_ROOT="$TOPDIR/sunxi-pack"
#PLATFORM="linux"
#PLATFORM="dragonboard"
PLATFORM="tina"

# change sd to emmc if board bootup with emmc and sdcard is used as storage.
STORAGE="sd"

pack_bootloader()
{
  BOARD=$1
  (
  echo "MACH=$MACH, PLATFORM=$PLATFORM, TARGET_PRODUCT=${TARGET_PRODUCT} BOARD=$BOARD"
  scripts/pack_img.sh -c ${MACH} -p ${PLATFORM} -b ${TARGET_PRODUCT} -d uart0 -s none -m normal -v none -t $TOPDIR
#  scripts/pack_img.sh -c $chip -p $platform -b $board -d $debug -s $sigmode -m $mode -v $securemode -t $T
  )
  $TOPDIR/scripts/bootloader.sh $BOARD
}

echo BPIMACH=$BPIMACH
for MACH in $BPIMACH ; do
  TARGET_PRODUCT=$MACH
  echo TARGET_PRODUCT=${TARGET_PRODUCT}
  BOARDS=`(cd sunxi-pack/allwinner/${TARGET_PRODUCT}/configs ; ls -1d BPI*)`
  for IN in $BOARDS ; do
    pack_bootloader $IN
  done 
done 

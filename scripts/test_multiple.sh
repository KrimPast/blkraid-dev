#!/bin/sh
BASEDIR=$(realpath "$(dirname "$0")")
ROOTDIR=$(realpath "$BASEDIR/..")
DATADIR="$ROOTDIR/data"

MODULE_NAME="blkraid"
MODULE_PATH="$ROOTDIR/build/src/${MODULE_NAME}.ko"
STORAGE1="/tmp/storage1.txt"
STORAGE2="/tmp/storage2.txt"
STORAGE3="/tmp/storage3.txt"
STORAGE1_CAPACITY=48KiB
STORAGE2_CAPACITY=64KiB
STORAGE3_CAPACITY=80KiB

INPUT="/tmp/input.txt"
OUTPUT="/tmp/output.txt"

CAPACITY=11KiB

echo "---Creating test storage(${STORAGE_CAPACITY})"
dd if=/dev/zero of=$STORAGE1 bs=${STORAGE1_CAPACITY} count=1 status=none
dd if=/dev/zero of=$STORAGE2 bs=${STORAGE2_CAPACITY} count=1 status=none
dd if=/dev/zero of=$STORAGE3 bs=${STORAGE3_CAPACITY} count=1 status=none

rm --force /dev/blkraid
echo "---Inserting module"
insmod ${MODULE_PATH} device_names=$STORAGE1,$STORAGE2,$STORAGE3

echo "---Driver status"
file "/dev/blkraid"

echo "---Creating test input($CAPACITY)"
dd if=/dev/random of=$INPUT bs=$CAPACITY count=1 status=none

echo "---Writing input into storage"
dd if=$INPUT of=/dev/${MODULE_NAME} bs=$CAPACITY count=1 status=none

dd if=/dev/${MODULE_NAME} of=$OUTPUT bs=$CAPACITY count=1 status=none

cmp -n $CAPACITY $INPUT $OUTPUT 
if (( $? == 0 )) ; then
	echo "Result: equals"
else
	echo "Result: differ"
fi
echo "---Detaching module"
sleep 0.5
rmmod "${MODULE_NAME}.ko"


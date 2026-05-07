#!/bin/sh
BASEDIR=$(realpath "$(dirname "$0")")
ROOTDIR=$(realpath "$BASEDIR/../..")
DATADIR="$ROOTDIR/data"

MODULE_NAME="blkraid"
MODULE_PATH="$ROOTDIR/build/src/${MODULE_NAME}.ko"
STORAGE="/tmp/storage.txt"
STORAGE_CAPACITY=64KiB

INPUT="/tmp/input.txt"
OUTPUT="/tmp/output.txt"
CAPACITY=32KiB

echo "---Creating test storage(${STORAGE_CAPACITY})"
dd if=/dev/zero of=$STORAGE bs=${STORAGE_CAPACITY} count=1 status=none

rm /dev/blkraid --force

echo "---Inserting module"
insmod ${MODULE_PATH} device_names=$STORAGE

echo "---Driver status"
file "/dev/blkraid"

rm $INPUT $OUTPUT --force

echo "---Creating test input($CAPACITY)"
dd if=/dev/random of=$INPUT bs=$CAPACITY count=1 status=none

echo "---Writing input into storage"
dd if=$INPUT of=/dev/${MODULE_NAME} bs=$CAPACITY count=1 status=none

echo "---Reading from storage"
dd of=$OUTPUT if=/dev/${MODULE_NAME} bs=$CAPACITY count=1 status=none

cmp -n $CAPACITY $INPUT $OUTPUT
if (( $? == 0 )) ; then
	echo "Result: equals"
else
	echo "Result: differ"
fi
echo "---Detaching module"
sleep 0.5
rmmod "${MODULE_NAME}.ko"


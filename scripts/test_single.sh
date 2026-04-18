#!/bin/sh
BASEDIR=$(realpath "$(dirname "$0")")
ROOTDIR=$(realpath "$BASEDIR/..")
DATADIR="$ROOTDIR/data"

MODULE_NAME="blkraid"
MODULE_PATH="$ROOTDIR/build/src/${MODULE_NAME}.ko"
STORAGE="/tmp/storage.txt"
STORAGE_CAPACITY=64KiB

INPUT="/tmp/input.txt"
OUTPUT="/tmp/output.txt"
CAPACITY=48KiB

echo "---Creating test storage(${STORAGE_CAPACITY})"
dd if=/dev/zero of=$STORAGE bs=${STORAGE_CAPACITY} count=1

echo "---Inserting module"
insmod ${MODULE_PATH} device_names=$STORAGE

rm $INPUT $OUTPUT --force

echo "---Creating test input($CAPACITY)"
dd if=/dev/random of=$INPUT bs=$CAPACITY count=1

echo "---Writing input into storage"
dd if=$INPUT of=/dev/${MODULE_NAME} bs=$CAPACITY count=1

echo "---Reading from storage"
dd of=$OUTPUT if=/dev/${MODULE_NAME} bs=$CAPACITY count=1

diff $INPUT $OUTPUT
if (( $? == 0 )) ; then
	echo "Result: equals"
else
	echo "Result: differ"
fi
echo "---Detaching module"
rmmod "${MODULE_NAME}.ko"


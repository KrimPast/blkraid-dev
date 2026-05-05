BASEDIR=$(realpath "$(dirname "$0")")
ROOTDIR=$(realpath "$BASEDIR/../..")

MODULE_NAME="blkraid"
MODULE_PATH="$ROOTDIR/build/src/${MODULE_NAME}.ko"

STORAGE1="/tmp/stor1"
STORAGE2="/tmp/stor2"
STORAGE3="/tmp/stor3"

dd if=/dev/random of="$STORAGE1" bs=24MiB count=1 
dd if=/dev/random of="$STORAGE2" bs=32MiB count=1
dd if=/dev/random of="$STORAGE3" bs=40MiB count=1 

echo "Storages have been successfully initialized!"

insmod ${MODULE_PATH} device_names=$STORAGE1,$STORAGE2,$STORAGE3

echo "Blkraid has been successfully initialized!"

mkfs.ext4 "/dev/$MODULE_NAME"
mkdir -p "/tmp/$MODULE_NAME"
mount "/dev/$MODULE_NAME" "/tmp/$MODULE_NAME"

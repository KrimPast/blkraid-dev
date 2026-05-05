BASEDIR=$(realpath "$(dirname "$0")")
ROOTDIR=$(realpath "$BASEDIR/../..")

MODULE_NAME="blkraid"
MODULE_PATH="$ROOTDIR/build/src/${MODULE_NAME}.ko"

STORAGE1="/tmp/stor1"
STORAGE2="/tmp/stor2"
STORAGE3="/tmp/stor3"

umount "/dev/$MODULE_NAME"
sleep 0.5
rmmod "/dev/$MODULE_NAME"

echo "Blkraid has been successfully deinited!"

rm -rf "/tmp/$MODULE_NAME"
rm -f $STORAGE1 $STORAGE2 $STORAGE3 
echo "Storages have been successfully deinited!"

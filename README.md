# blkraid
This block driver implements RAID-1
Now, write and read work only for one device successfully.
Kernel version: `6.18.16-200.fc43.x86_64`
## Usage
1. Build 
```sh 
make
```
2. Insert. Device may be file, partition or disk
```sh
sudo insmod build/src/blkraid.ko device_names=<device_1, ..., device_8>
```
3. [Optional] You can see inserted device 
```sh
lsblk
```
4. Write
```sh
dd if=<input_file> of=/dev/blkraid bs=<size> count=1
```
5. Read
```sh
dd if=/dev/blkraid of=<output_file> bs=<size> count=1
```
6. Remove
```sh
sudo rmmod blkraid.ko
```
## About code
### Known bugs
- Writing in unit, when were connected multiple devices, causes wrapped output
### Nearest plans
- Fixing bug
- Implement reading from multiple devices
### Possible future
- Implement dynimic insert and remove devices without reload of driver
- Make support of RAID-0

# blkraid
This block driver implements RAID-1. Kernel version: `6.18.16-200.fc43.x86_64`

Now, write and read work **only for one** device successfully. 

## Usage
1. **Build** 
	```sh 
	make
	```
2. **Insert**. Device may be file, partition or disk. Names **must be** separated by commas, **without** spaces
	```sh
	sudo insmod build/src/blkraid.ko device_names=device
	```
	or
	```sh
	sudo insmod build/src/blkraid.ko device_names=device1,device2,...,device8
 	```
4. _(Optional). You can see inserted device:_ 
	```sh
	lsblk
	```
5. **Write**
	```sh
	dd if=input_file of=/dev/blkraid bs=amount_bytes count=1
	```
6. **Read**
	```sh
	dd if=/dev/blkraid of=output_file bs=amount_bytes count=1
	```
7. **Remove**
	```sh
	sudo rmmod blkraid.ko
	```
## About code
### Known bugs
- Writing in unit, when were connected multiple devices, causes wrapped output
### Nearest plans
- Fix bug
- Implement fast reading from multiple devices
### Possible future
- Implement dynimic insert and remove devices without reload of driver
- Make support of RAID-0

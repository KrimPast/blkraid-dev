# blkraid
This block driver implements RAID-1. 

Kernel version: `6.18.16-200.fc43.x86_64`

## Usage
### Necessary
1. **Build** 
	```sh 
	make
	```
2. **Insert**. Devices may be files, partitions or disks. Names **must be** separated by commas, **without** spaces
	```sh
	sudo insmod build/src/blkraid.ko device_names=device1,device2,...,device_n
 	```
3. **Write**
	```sh
	dd if=input_file of=/dev/blkraid bs=amount_bytes count=1
	```
4. **Read**
	```sh
	dd if=/dev/blkraid of=output_file bs=amount_bytes count=1
	```
5. **Remove**
	```sh
	sudo rmmod blkraid.ko
	```
### Extra
1. When device inserted, you can see it in devices list:
	```sh
	lsblk
	```
3. You can see info about device:
	```sh
 	modinfo build/src/blkraid.ko
 	```
4. Dynamic managing devices (when inserted)
   
	Before next steps:
	```sh
 	cd /sys/module/blkraid/parameters/
 	```
 	- Show inserted devices
		```sh
		cat ./device_names
  		```
	- Adding
    	```sh
     	echo -n device_name > ./add_device 
    	```
	- Removing
   		```sh
		echo -n device_name > ./remove_device
     	```
## About code
### Implemented
- Read/write of block driver
- Dynamic insert and remove devices without reload of driver
- Auto RAID size equals minimum of sizes of all disks
### Known bugs
- Writing more than half of all size may cause extending size of file
### Possible future
- Implement fast reading from multiple devices
- Make support of RAID-0

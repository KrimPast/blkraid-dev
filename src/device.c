#include "include/device.h"

struct file* device_open(char* device_name){
	struct file* new_file = filp_open(device_name, O_RDWR | O_LARGEFILE, 0);
	return new_file;
}
loff_t device_get_capacity(struct file* device){
	return i_size_read(file_inode(device));
}

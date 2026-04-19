// Copyright 2026 | Nikita Egorov
// This file is part of blkraid.

// blkraid is free software: you can redistribute it and/or modify it under the terms
// of the GNU General Public License as published by the Free Software Foundation, 
// either version 2 of the License, or (at your option) any later version.

// blkraid is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
// without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
// PARTICULAR PURPOSE. See the GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along with blkraid. 
// If not, see <https://www.gnu.org/licenses/>.

#include "include/device.h"

struct file* device_open(const char* device_name){
	struct file* new_file = filp_open(device_name, O_RDWR | O_LARGEFILE, 0);
	return new_file;
}
loff_t device_get_capacity(struct file* device){
	return i_size_read(file_inode(device));
}
// file1 is valid, we want check that file2 may point to file1's device
bool device_is_not_broken_and_equals(struct file* file1, const char* file_name2){
	struct file* file2 = device_open(file_name2);
	if(IS_ERR(file2)){
		mpr_err("Proposed device to add is broken\n");
		return false;
	}
	struct inode *inode1 = file_inode(file1);
    	struct inode *inode2 = file_inode(file2);
	bool res = (inode1 == inode2);
	filp_close(file2, NULL);

	return res;
}

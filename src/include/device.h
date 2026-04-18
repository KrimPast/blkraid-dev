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

#include <linux/types.h>
#include <linux/stat.h>
#include <linux/fs.h>

#ifndef DEVICE_H
#define DEVICE_H

struct file* device_open(char*);
loff_t device_get_capacity(struct file*);

#endif

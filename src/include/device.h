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

#define DEVICE_NAME "blkraid"

#define mpr_err(...) pr_err(DEVICE_NAME ": " __VA_ARGS__)
#define mpr_warn(...) pr_warn(DEVICE_NAME ": " __VA_ARGS__)
#define mpr_info(...) pr_info(DEVICE_NAME ": " __VA_ARGS__)

#define MAX_DEV_LENGTH 32

struct file* device_open(const char*);
loff_t device_get_capacity(struct file*);
bool device_is_not_broken_and_equals(struct file*, const char*);

#endif

#include <linux/types.h>
#include <linux/stat.h>
#include <linux/fs.h>

#ifndef DEVICE_H
#define DEVICE_H

struct file* device_open(char*);
loff_t device_get_capacity(struct file*);

#endif

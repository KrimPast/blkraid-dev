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
 
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>

#include "include/device.h"

#define SCTR_SIZE 512
#define MAX_CAPACITY 2147483648

#define MAX_DEVICES 64

static char* device_names[MAX_DEVICES];
static struct file* devices[MAX_DEVICES];
static int init_amount;
static int curr_amount = 0;

static struct block_dev{
	struct blk_mq_tag_set tag_set;
	struct request_queue *queue;
	struct gendisk *gd;
	loff_t capacity;
	int major;
} dev;

static blk_status_t queue_rq(struct blk_mq_hw_ctx *hctx, const struct blk_mq_queue_data *bd){
	struct request *req = bd->rq;

	struct block_dev* curr_dev = hctx->queue->queuedata;
	loff_t start_pos = (loff_t)blk_rq_pos(req) * SCTR_SIZE;
        loff_t pos = start_pos;
        unsigned long len = blk_rq_bytes(req);
        int dir = rq_data_dir(req);
        struct bio_vec bvec;
        struct req_iterator iter;
	
	mpr_info("request started, pos = %lld, dir = %s\n", pos, dir == WRITE ? "WRITE" : "READ");
	blk_mq_start_request(req);

	if (pos + len > curr_dev->capacity * SCTR_SIZE) {
                mpr_info("request ended with IO error\n");
                blk_mq_end_request(req, BLK_STS_IOERR);
                return BLK_STS_IOERR;
        }
	int req_counter = 0;
	if(dir == WRITE){
		for (int i = 0; i < curr_amount; i++){
			pos = start_pos;
			rq_for_each_segment(bvec, req, iter) {
                		void *buffer = page_address(bvec.bv_page) + bvec.bv_offset;
                		size_t count = bvec.bv_len;

				int ret = kernel_write(devices[i], buffer, count, &pos);
				if(ret < 0){
					mpr_warn("cannot write into block device(#%d). Error code: %d\n", req_counter, ret);
				}
				++req_counter;
			}
			vfs_fsync(devices[i], 0);
		}
	} else { // dir == READ
		rq_for_each_segment(bvec, req, iter) {
                	void *buffer = page_address(bvec.bv_page) + bvec.bv_offset;
                	size_t count = bvec.bv_len;

			int ret = kernel_read(devices[0], buffer, count, &pos);
			if(ret < 0){
				mpr_warn("cannot read from block device(%d)\n", req_counter);
			}
			//mpr_info("segment pos: %lld\n", pos);
			++req_counter;
        	}
	}
	blk_mq_end_request(req, BLK_STS_OK);
	mpr_info("request ended, pos = %lld\n", pos);
	return BLK_STS_OK;
}

static int open(struct gendisk *gd, blk_mode_t mode){
        return 0;
}
static void release(struct gendisk *gd) { }
static int ioctl(struct block_device *bdev, blk_mode_t mode,
                unsigned int cmd, unsigned long arg){
        return -ENOTTY;
}
static const struct block_device_operations fops = {
	.owner = THIS_MODULE,
        .open = open,
        .release = release,
        .ioctl = ioctl
};
static const struct blk_mq_ops mq_ops = {
        .queue_rq = queue_rq,
};

static void update_capacity(loff_t new_capacity){
	mpr_info("Updated RAID capacity: %lld --> %lld(now)\n", dev.capacity, new_capacity);
	dev.capacity = new_capacity;
	set_capacity(dev.gd, dev.capacity);
}
static int add_device(const char *arg, const struct kernel_param *kp){
	if (init_amount == 0){
		mpr_err("You mustn't invoke add_device() in init period\n");
		return -EINVAL;
	}
	if (curr_amount + 1 > MAX_DEVICES){
		mpr_err("You cannot add new device. %d is roof limit\n", MAX_DEVICES);
		return -EINVAL;
	}

	for(int i = 0; i < curr_amount; i++){
		if(device_is_not_broken_and_equals(devices[i], arg)){
			mpr_err("%s already exists in RAID\n", arg);
			return -EINVAL;
		}
	}
	struct file* new_device = device_open(arg);
	if(IS_ERR(new_device)){
		mpr_err("%s is not correct file name\n", arg);
		return -EINVAL;
	}
	
	char* name = kzalloc(sizeof(char) * MAX_DEV_LENGTH, GFP_KERNEL);
	if(!name){
		pr_err("Cannot allocate memory for save file name\n");
		return -ENOMEM;
	}
	strcpy(name, arg);
	
	devices[curr_amount] = new_device;
	device_names[curr_amount] = name;
	curr_amount++;

	loff_t file_capacity = device_get_capacity(new_device) / SCTR_SIZE;
	mpr_info("%s is successfully added!\n", arg);
	mpr_info("New amount of devices: %d\n", curr_amount);
	
	if(file_capacity < dev.capacity){
		update_capacity(file_capacity);
	}
	return 0;
}
static const struct kernel_param_ops add_device_ops = {
	.set = add_device
};
static int remove_device(const char *arg, const struct kernel_param *kp){
	int ind = -1;
	for(int i = 0; i < curr_amount; i++){
		if(device_is_not_broken_and_equals(devices[i], arg)){
			ind = i;
			break;
		}
	}
	if(ind == -1){
		mpr_err("Device to delete is not found\n");
		return -EINVAL;
	}
	filp_close(devices[ind], NULL);
	kfree(device_names[ind]);
	for(int i = ind + 1; i < curr_amount; i++){
		device_names[i - 1] = device_names[i];
		devices[i - 1] = devices[i];
	}
	curr_amount--;
	mpr_err("%s is successfully deleted\n", arg);

	loff_t new_capacity = curr_amount > 0 ? MAX_CAPACITY : 0;
	for(int i = 0; i < curr_amount; i++){
		new_capacity = min(new_capacity, device_get_capacity(devices[i]));
	}
	new_capacity /= SCTR_SIZE;
	if(new_capacity == 0 || new_capacity > dev.capacity){
		update_capacity(new_capacity);
	}
	return 0;
}
static const struct kernel_param_ops remove_device_ops = {
	.set = remove_device
};
static int __init blk_init(void){
	// Thing that is needed to correct display the devices list in "/sys/module/blkraid/parameters/"
	init_amount = curr_amount;
	curr_amount = 0;
	
	mpr_info("Initial amount of devices: %d", init_amount);
	loff_t capacity = MAX_CAPACITY;
	for(int i = 0; i < init_amount; i++){
		int ret = add_device(device_names[i], NULL);
		if (ret != 0) return ret;

		loff_t dev_capacity = device_get_capacity(devices[i]) / SCTR_SIZE;
		mpr_info("Device %d: %s. Capacity: %lld sectors\n", i, device_names[i], dev_capacity);
		capacity = min(capacity, dev_capacity);
	}
	dev.capacity = curr_amount > 0 ? capacity : 0;
	mpr_info("Final RAID capacity: %lld sectors\n", dev.capacity);

	dev.tag_set.ops = &mq_ops;
        dev.tag_set.driver_data = &dev;
        dev.tag_set.queue_depth = 128;
        dev.tag_set.nr_hw_queues = 1;
        dev.tag_set.nr_maps = 1;
        dev.tag_set.numa_node = NUMA_NO_NODE;
        dev.tag_set.flags = 0;
        dev.tag_set.cmd_size = 0;

        int ret = blk_mq_alloc_tag_set(&dev.tag_set);
        if (ret) {
                mpr_err("Cannot allocate tag set\n");
                goto end;
        }

        dev.major = register_blkdev(0, DEVICE_NAME);
        if (dev.major < 0) {
                mpr_err("Cannot register device in system\n");
                goto free_tag_set;
        }

        dev.gd = blk_mq_alloc_disk(&dev.tag_set, NULL, &dev);
        if (IS_ERR(dev.gd)) {
                mpr_err("Cannot allocate disk\n");
                ret = PTR_ERR(dev.gd);
                goto unregister_blk;
        }
	
	dev.queue = dev.gd->queue;
	dev.queue->queuedata = &dev;
	
	dev.queue->limits.logical_block_size = SCTR_SIZE;
        dev.queue->limits.physical_block_size = SCTR_SIZE;

	dev.gd->fops = &fops;
	snprintf(dev.gd->disk_name, 32, DEVICE_NAME);

	set_capacity(dev.gd, dev.capacity);
	ret = add_disk(dev.gd);
        if (ret) {
                mpr_err("Cannot add disk\n");
                goto put_disk;
        }
	mpr_info("Driver was successfully added!\n");
	return 0;

put_disk:
	put_disk(dev.gd);
unregister_blk:
	unregister_blkdev(dev.major, DEVICE_NAME);
free_tag_set:
        blk_mq_free_tag_set(&dev.tag_set);
end:
	return -1;
}
static void __exit blk_exit(void){
	for(int i = 0; i < curr_amount; i++){
		kfree(device_names[i]);
		filp_close(devices[i], NULL);
        }
	del_gendisk(dev.gd);
	blk_mq_free_tag_set(&dev.tag_set);
	put_disk(dev.gd);
	mpr_info("Driver was successfully unloaded!\n");
}

MODULE_PARM_DESC(add_device, "In runtime: add device");
module_param_cb(add_device, &add_device_ops, NULL, S_IRUGO | S_IWUSR);

MODULE_PARM_DESC(remove_device, "In runtime: remove device");
module_param_cb(remove_device, &remove_device_ops, NULL, S_IRUGO | S_IWUSR);

MODULE_PARM_DESC(device_names, "In inserting module: add list of devices / In runtime: get devices list");
module_param_array(device_names, charp, &curr_amount, S_IRUGO);
module_init(blk_init);
module_exit(blk_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Nikita Egorov <avelparmen@gmail.com>");
MODULE_DESCRIPTION("RAID-1 driver. In development");

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

#define MAX_DEVICES 8
#define DEVICE_NAME "blkraid"

#define mpr_err(...) pr_err(DEVICE_NAME ": " __VA_ARGS__)
#define mpr_warn(...) pr_warn(DEVICE_NAME ": " __VA_ARGS__)
#define mpr_info(...) pr_info(DEVICE_NAME ": " __VA_ARGS__)

static char* device_names[MAX_DEVICES];
static struct file* devices[MAX_DEVICES];
static int devices_amount;

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
		for (int i = 0; i < devices_amount; i++){
			pos = start_pos;
			rq_for_each_segment(bvec, req, iter) {
                		void *buffer = page_address(bvec.bv_page) + bvec.bv_offset;
                		size_t count = bvec.bv_len;

				int ret = kernel_write(devices[i], buffer, count, &pos);
				if(ret < 0){
					mpr_warn("cannot write into block device(#%d). Error code: %d\n", req_counter, ret);
				}
				++req_counter;
				pos += count;
			}
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
			pos += count;
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

static int add_new_device(const char *arg, const struct kernel_param *kp){
	for(int i = 0; i < devices_amount; i++){
		if(device_is_equals(device_names[i], arg)){
			mpr_err("%s is already exists in RAID", arg);
			return -EINVAL;
		}
	}
	struct file* new_device = device_open(arg);
	if(IS_ERR(new_device)){
		mpr_err("%s is not correct file name\n", arg);
		return -EINVAL;
	}
	devices[devices_amount++] = new_device;

	loff_t file_capacity = device_get_capacity(new_device) / SCTR_SIZE;
	mpr_info("%s is successfully added!\n", arg);
	mpr_info("New amount of devices: %d\n", devices_amount);
	
	if(file_capacity < dev.capacity){
		mpr_info("Updated RAID capacity: %lld --> %lld(now)\n", dev.capacity, file_capacity);
		dev.capacity = file_capacity;
		set_capacity(dev.gd, dev.capacity);
	}
	return 0;
}
static const struct kernel_param_ops add_device_ops = {
	.set = add_new_device,
};

static int __init blk_init(void){
	pr_info("Amount of devices: %d", devices_amount);
	loff_t capacity = MAX_CAPACITY;
	for(int i = 0; i < devices_amount; i++){
		devices[i] = device_open(device_names[i]);
		if (IS_ERR(devices[i])){
			mpr_err("%s is not correct file name\n", device_names[i]);
			return -EINVAL;
		}
		loff_t dev_capacity = device_get_capacity(devices[i]) / SCTR_SIZE;
		mpr_info("Device %d: %s. Capacity: %lld sectors\n", i, device_names[i], dev_capacity);
		capacity = min(capacity, dev_capacity);
	}
	dev.capacity = devices_amount > 0 ? capacity : 0; 
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
	for(int i = 0; i < devices_amount; i++){
                filp_close(devices[i], NULL);
        }
	del_gendisk(dev.gd);
	blk_mq_free_tag_set(&dev.tag_set);
	put_disk(dev.gd);
	mpr_info("Driver was successfully unloaded!\n");
}

MODULE_PARM_DESC(add_device, "In runtime: add new device");
module_param_cb(add_device, &add_device_ops, NULL, S_IRUGO | S_IWUSR);

MODULE_PARM_DESC(device_names, "In inserting module: add list of devices");
module_param_array(device_names, charp, &devices_amount, 0644);
module_init(blk_init);
module_exit(blk_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Nikita Egorov <avelparmen@gmail.com>");
MODULE_DESCRIPTION("RAID-1 driver. In development");

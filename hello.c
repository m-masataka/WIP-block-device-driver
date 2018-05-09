//#include <linux/blk_types.h>

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h> /* printk() */
#include <linux/fs.h>     /* everything... */
#include <linux/errno.h>  /* error codes */
#include <linux/types.h>  /* size_t */
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>

#include <linux/bio.h>

#define KERNEL_SECTOR_SIZE 512

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Satoru Takeuchi <satoru.takeuchi@gmail.com>");
MODULE_DESCRIPTION("Hello world kernel module");

static int hardsect_size = 512;
static int logical_block_size = 512;
static int nsectors = 1024;
static int major_num = 0;
char* buff = NULL;

static struct request_queue *Queue;

static struct sbd_device {
	unsigned long size;
	spinlock_t lock;
	u8 *data;
	struct gendisk *gd;
} Device;

int sbd_getgeo(struct block_device * block_device, struct hd_geometry * geo) {
	long size;

	/* We have no real geometry, of course, so make something up. */
	size = nsectors*hardsect_size * (logical_block_size / KERNEL_SECTOR_SIZE);
	geo->cylinders = (size & ~0x3f) >> 6;
	geo->heads = 4;
	geo->sectors = 16;
	geo->start = 0;
	return 0;
}

static struct block_device_operations sbd_ops = {
	.owner = THIS_MODULE,
	.open    = NULL,
	.release = NULL,
	.ioctl   = NULL,
};


/*static void sbd_transfer( unsigned long sector, unsigned long nsect, char *buffer, int write) {
	unsigned long offset = sector * hardsect_size;
	unsigned long nbytes = nsect*hardsect_size;
	unsigned long device_size = nsectors * hardsect_size;
	printk("ok11\n");
	if ((offset + nbytes) > device_size) {
		printk (KERN_NOTICE "sbd: Beyond-end write (%ld %ld)\n", offset, nbytes);
		return;
	}
	printk("ok12\n");
	if (write) {
		printk("ok13\n");
		memcpy(device->data + offset, buffer, nbytes);
		printk("ok14\n");
	} else {
		printk("ok15\n");
		memcpy(buffer, device->data + offset, nbytes);
		printk("ok16\n");
	}
	printk("ok17\n");
}*/

static void sbd_request(struct request_queue *q) {
	struct request *req;

	req = blk_fetch_request(q);
	printk("ok1\n");
	while (req != NULL) {
		printk("ok2\n");
		if (req == NULL || (req->cmd_type != REQ_TYPE_FS))  {
			printk("ok3\n");
			printk (KERN_NOTICE "Skip non-CMD request\n");
			__blk_end_request_all(req, -EIO);
			printk("ok4\n");
			continue;
		}else{
			printk("ok5\n");
			unsigned long offset = blk_rq_pos(req)*hardsect_size;
			unsigned long nbytes = blk_rq_cur_sectors(req)*hardsect_size;
			unsigned long device_size = blk_rq_cur_sectors(req) * hardsect_size;
			char *buffer = bio_data(req->bio);
			if ((offset + nbytes) > device_size) {
				printk (KERN_NOTICE "sbd: Beyond-end write (%ld %ld)\n", offset, nbytes);
			}else {
				if ( rq_data_dir(req) ) {
					memcpy(buff + offset, buffer, nbytes);
				} else {
					memcpy(buffer, buff + offset, nbytes);
				}
			}
		}
		printk("ok7\n");
		if ( ! __blk_end_request_cur(req, 0) ) {
			printk("ok8\n");
			req = blk_fetch_request(q);
			printk("ok9\n");
		}
		printk("ok10\n");
	}
}

static int mymodule_init(void) {
	// init device 
	struct sbd_device Device;
	Device.size = nsectors * hardsect_size;
	spin_lock_init(&Device.lock);
	buff = vmalloc(Device.size);
	if (buff == NULL)
		return -ENOMEM;
	printk("device size %d\n", Device.size);
	// get request queue
	Queue = blk_init_queue(sbd_request, &Device.lock);
	if (Queue == NULL)
		goto out;
	blk_queue_logical_block_size(Queue, hardsect_size);
	// register
	major_num = register_blkdev(major_num, "test");
	printk("major_num %d\n", major_num);
	if (major_num < 0) {
		printk(KERN_WARNING "sbd: unable to get major number\n");
		goto out;
	}
	// gendisk
	Device.gd = alloc_disk(16);
	if (! Device.gd)
		goto out_unregister;
	Device.gd->major = major_num;
	Device.gd->first_minor = 0;
	Device.gd->fops = &sbd_ops;
	Device.gd->private_data = &Device;
	strcpy (Device.gd->disk_name, "test");
	set_capacity(Device.gd, nsectors*(hardsect_size/KERNEL_SECTOR_SIZE));
	Device.gd->queue = Queue;
	add_disk(Device.gd);
	printk( KERN_INFO "modtest is loaded\n" );
        return 0;
	out_unregister:
		unregister_blkdev(major_num, "test");
	out:
		vfree(buff);
		return -ENOMEM;
		
}

static void mymodule_exit(void) {
        del_gendisk(Device.gd);
	put_disk(Device.gd);
	unregister_blkdev(major_num, "test");
	blk_cleanup_queue(Queue);
	vfree(buff);
}

module_init(mymodule_init);
module_exit(mymodule_exit);

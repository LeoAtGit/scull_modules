#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/slab.h>

#include "scull.h"

int scull_major = 0;
int scull_minor = 0;

int device_num = 0;

struct scull_device *scull_device;

struct file_operations scull_fops = {
	.owner = THIS_MODULE,
	.read = scull_read,
	.write = scull_write,
	.open = scull_open,
	.release = scull_release,
};

ssize_t scull_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	return 0;
}

ssize_t scull_write(struct file *filp, const char *buf, size_t count, loff_t *fpos)
{
	return 1;
}

int scull_open(struct inode *inode, struct file *filp)
{
	return 0;
}

int scull_release(struct inode *inode, struct file *filp)
{
	return 0;
}

int scull_init(void)
{
	int err;
	struct cdev *scull_cdev;
	
	err = alloc_chrdev_region(&device_num, scull_minor, 1, "scull");
	PDEBUG("%d\n", device_num);
	if (err)
		return -1;
	scull_major = MAJOR(device_num);
	scull_minor = MINOR(device_num);

	scull_device = kmalloc(sizeof(struct scull_device), GFP_KERNEL);
	if (!scull_device)
		return -ENOMEM;

	scull_cdev = cdev_alloc();
	scull_device->cdev = scull_cdev;
	scull_device->size = 0;

	cdev_init(scull_cdev, &scull_fops);
	scull_cdev->owner = THIS_MODULE;

	err = cdev_add(scull_cdev, device_num, 1);
	if (err)
		return -2;

	PDEBUG("Loaded succesfully.\n");

	return 0;
}

void scull_exit(void)
{
	cdev_del(scull_device->cdev);
	unregister_chrdev_region(device_num, 1);

	if (scull_device)
		kfree(scull_device);

	PDEBUG("Unloaded succesfully.\n");

	return;
}

module_init(scull_init);
module_exit(scull_exit);

MODULE_AUTHOR("Leo Sperling");
MODULE_LICENSE("GPL");


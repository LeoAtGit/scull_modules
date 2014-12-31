#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/slab.h>

#include "scull0.h"

int scull_major = 0;
int scull_minor = 0;

int dev_num = 0; /* The number of the device */

struct scull_device *scull_device;

struct file_operations scull_fops = {
	.owner = THIS_MODULE,
	//.read = scull_read,
	//.write = scull_write,
//	.open = scull_open,
	.open = NULL,
	//.release = scull_release,
};

static void scull_cdev_init(struct scull_device *dev)
{
	int err_code = 0;

	cdev_init(&dev->cdev, &scull_fops);
	dev->cdev.owner = THIS_MODULE;
	//dev->cdev.ops = &scull_fops  /* TODO check if I even need this line */
	
	err_code = cdev_add(&dev->cdev, dev_num, 1);
	if(err_code)
		printk(KERN_NOTICE "Error %d: Error adding scull0!\n", err_code);
}

static void scull_cdev_del(struct scull_device *dev)
{
	cdev_del(&dev->cdev);
}

int scull_init(void)
{
	int err = 0;
	struct cdev *scull_cdev;

	printk(KERN_DEBUG "Attempting to load the module\n");

	err = alloc_chrdev_region(&dev_num, scull_minor, 1, "scull");
	if (err)
		return -1;
	scull_major = MAJOR(dev_num);
	scull_minor = MINOR(dev_num);

	scull_device = kmalloc(sizeof(struct scull_device), GFP_KERNEL);
	if (!scull_device)
		return -ENOMEM;

	scull_cdev = cdev_alloc();
	//my_chdev->ops = &scull_fops; /* TODO Check if i need this line */
	scull_device->cdev = *scull_cdev;

	scull_cdev_init(scull_device);

	printk(KERN_ERR "LOADED\n");
	return err;
}

void scull_clean_up(void)
{
	scull_cdev_del(scull_device);
	if (scull_device)
		kfree(scull_device);

	unregister_chrdev_region(dev_num, 1);

	printk(KERN_ERR "UNLOADED\n");
}

MODULE_AUTHOR("Leo Sperling");
MODULE_LICENSE("GPL");

module_init(scull_init);
module_exit(scull_clean_up);


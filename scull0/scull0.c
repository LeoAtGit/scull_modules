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
	.read = scull_read,
	.write = scull_write,
	.open = scull_open,
	.release = scull_release,
};

ssize_t scull_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	struct scull_device *dev = filp->private_data;
	ssize_t ret = 0;

	printk(KERN_NOTICE "entered scull_read\n");
	//if (*f_pos >= dev->size) /* Check how the size is implemented */
	
	if (!dev->data) /* if this is NULL then dont allocate any space */
		goto out;

	if (count > dev->size)
		count = dev->size;

	if (copy_to_user(buf, dev->data, count)){ 
		ret = -EFAULT;
		goto out;
	}

	*f_pos += count; 
	ret = count;

out:
	return ret;
}

ssize_t scull_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	struct scull_device *dev = filp->private_data;
	ssize_t ret = 0;

	printk(KERN_NOTICE "entered scull_write\n");

	if (dev->data)
		kfree(dev->data);

	dev->data = kmalloc(count * sizeof(char), GFP_KERNEL);
	if (!dev->data){
		ret = -ENOMEM;
		goto out;
	}
	memset(dev->data, 0, count * sizeof(char));
	dev->size = count;

	if (copy_from_user(dev->data, buf, count)){
		ret = -EFAULT;
		goto out;
	}

	*f_pos += count;
	ret = count;

out:
	return ret;
}

int scull_open(struct inode *inode, struct file *filp)
{
	struct scull_device *dev;

	printk(KERN_NOTICE "entered scull_open\n");
	dev = container_of(inode->i_cdev, struct scull_device, cdev);
	filp->private_data = dev;

	return 0;
}

int scull_release(struct inode *inode, struct file *filp)
{
	printk(KERN_NOTICE "entered scull_release\n");
	return 0;
}

void scull_cdev_init(struct scull_device *dev)
{
	int err_code = 0;

	printk(KERN_NOTICE "entered scull_cdev_init\n");
	cdev_init(&dev->cdev, &scull_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->data = NULL;
	dev->size = 0;
	//dev->cdev.ops = &scull_fops  /* TODO check if I even need this line */
	
	err_code = cdev_add(&dev->cdev, dev_num, 1);
	if(err_code)
		printk(KERN_NOTICE "Error %d: Error adding scull0!\n", err_code);
}

void scull_cdev_del(struct scull_device *dev)
{
	printk(KERN_NOTICE "entered scull_cdev_del\n");
	printk(KERN_ERR "[DEBUG] we are now in scull_cdev_del\n");

	if (dev->data)
		kfree(dev->data);

	printk(KERN_ERR "[DEBUG] I could free dev->data without a segfault\n");

	cdev_del(&dev->cdev);

	return;
}

int scull_init(void)
{
	int err = 0;
	struct cdev *scull_cdev;

	printk(KERN_NOTICE "entered scull_init\n");
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
	printk(KERN_NOTICE "entered scull_clean_up\n");

	scull_cdev_del(scull_device);

	printk(KERN_ERR "[DEBUG] scull_cdev_del executed correctly\n");

	if (scull_device)
		kfree(scull_device);

	printk(KERN_ERR "[DEBUG] kfreed the scull_device global variable\n");

	unregister_chrdev_region(dev_num, 1);

	printk(KERN_ERR "UNLOADED\n");
}

MODULE_AUTHOR("Leo Sperling");
MODULE_LICENSE("GPL");

module_init(scull_init);
module_exit(scull_clean_up);


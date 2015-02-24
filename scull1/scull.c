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
	int read = count;

	/* FIXME
	 * use down_interruptible so the device can be interrupted by user
	 * space interrupts */
	down(scull_device->sem);

	if (count > scull_device->size)
		count = scull_device->size;

	if (copy_to_user(buf, scull_device->data, count)){
		read = -EFAULT;
		goto out;
	}

	read -= count;
out:
	/* FIXME
	 * The bug here is that the lock is not interruptible by user space 
	 * */
	//up(scull_device->sem);
	return read;
}

ssize_t scull_write(struct file *filp, const char *buf, size_t count, loff_t *fpos)
{
	int written = 0;
	char *data = NULL;

	if (down_interruptible(scull_device->sem))
		return -ERESTARTSYS;

	if (scull_device->data)
		kfree(scull_device->data);

	if (count > SCULL_SIZE)
		count = SCULL_SIZE;
	
	scull_device->data = kmalloc(count * sizeof(char) + 1, GFP_KERNEL);
	if (!scull_device->data){
		written = -ENOMEM;
		goto out;
	}
	scull_device->size = count;

	data = scull_device->data;
	*(data + count + 1) = '\0';

	if (copy_from_user(scull_device->data, buf, count)){
		written = -EFAULT;
		goto out;
	}

	written = count;
out:
	up(scull_device->sem);
	return written;
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
	scull_device->size = SCULL_SIZE;
	scull_device->data = NULL;

	cdev_init(scull_cdev, &scull_fops);
	scull_cdev->owner = THIS_MODULE;

	scull_device->sem = kmalloc(sizeof(struct semaphore), GFP_KERNEL);
	if (!scull_device->sem)
		return -ENOMEM;

	sema_init(scull_device->sem, 1);
	//init_MUTEX(scull_device->sem);

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

	if (scull_device->data)
		kfree(scull_device->data);
	
	if (scull_device->sem)
		kfree(scull_device->sem);

	if (scull_device)
		kfree(scull_device);

	PDEBUG("Unloaded succesfully.\n");

	return;
}

module_init(scull_init);
module_exit(scull_exit);

MODULE_AUTHOR("Leo Sperling");
MODULE_LICENSE("GPL");


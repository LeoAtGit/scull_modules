#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h> // for kmalloc and kfree
#include <linux/fs.h> // for file_operations
#include <asm/uaccess.h> // for access_ok and put_user and get_user
#include <linux/cdev.h> // obviously for cdev stuff

#include <asm/ioctl.h>

#include "scull.h"

#define MAX_STR_LEN 128

// this is the data byte everything will be saved upon
char *data = NULL;

int scull_major = 0;
int scull_minor = 0;

int device_num = 0;

struct cdev *scull_cdev = NULL;

struct file_operations scull_fops = {
	.owner = THIS_MODULE,
	.read = scull_read,
	.open = scull_open,
	.release = scull_release,
	.unlocked_ioctl = scull_ioctl,
};

long scull_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	int *tmp = NULL;

	/* extract the type and the number bitfields, and sort out bad 
	 * cmds: return ENOTTY */

	if (_IOC_TYPE(cmd) != SCULL_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > SCULL_IOC_MAXNR) return -ENOTTY;

	/* the direction is a bitmask, and VERIFY_WRITE catches R/W
	 * transfers. "Type" is user-oriented, while
	 * access_ok is kernel-oriented, so the concept of read and write
	 * is reversed! */

	if (_IOC_DIR(cmd) & _IOC_READ)
		ret = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		ret = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));

	if (ret)
		return -EFAULT;

	switch(cmd) {
		case SCULL_IOCSDATA: 
			if (!capable(CAP_SYS_ADMIN))
				return -EPERM;
			// here you could take __get_user because access_ok was 
			// already checked on that pointer.
			ret = get_user(tmp, (int __user *)arg);

			PDEBUG("address of tmp: %p\naddress of arg: %p\naddress of data: %p\n", tmp, (void __user *)arg, data);

			break;
		case SCULL_IOCTDATA: 
			break;
		case SCULL_IOCGDATA: 
			break;
		case SCULL_IOCQDATA: 
			break;
		case SCULL_IOCXDATA: 
			break;
		case SCULL_IOCHDATA: 
			break;
		default: 
			return -ENOTTY;
	}

	return ret;
}

ssize_t scull_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	return count; 
}

int scull_open(struct inode *inode, struct file *filp)
{
	PDEBUG("scull_open called, return 0 now\n");
	return 0;
}

int scull_release(struct inode *inode, struct file *filp)
{
	return 0;
}

int scull_init(void)
{
	int err;

	err = alloc_chrdev_region(&device_num, scull_minor, 1, "scull");
	if (err)
		return -1;

	PDEBUG("device number = %d\n", device_num);

	scull_major = MAJOR(device_num);
	scull_minor = MINOR(device_num);

	scull_cdev = cdev_alloc();
	cdev_init(scull_cdev, &scull_fops);

	data = kmalloc( sizeof(char) * MAX_STR_LEN, GFP_KERNEL);
	strcpy(data, "This is the value of the global variable.\n\0");
	PDEBUG("This is the pointer address: %p\n", data);

	return 0;
}

void scull_exit(void)
{
	cdev_del(scull_cdev);
	unregister_chrdev_region(device_num, 1);

	kfree(data);

	return;
}

module_init(scull_init);
module_exit(scull_exit);

MODULE_AUTHOR("Leo Sperling");
MODULE_LICENSE("GPL");


#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/slab.h>
#include <linux/fs.h>

#include <linux/cdev.h>

#include <asm/uaccess.h>

#include <linux/wait.h>
#include <linux/sched.h>

char *data = NULL;

int scull_major = 0;
int scull_minor = 0;

int device_num = 0;

struct cdev *scull_cdev = NULL;

//wait_queue_head_t scull_queue;
DECLARE_WAIT_QUEUE_HEAD(scull_queue);

ssize_t scull_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	// other possibilities of sleeps:
	//   - wait_event (not interruptible -> bad)
	//   - wait_event_interruptible (preferred)
	//   - wait_event_timeout(queue, condition, timeout)
	//   - wait_event_interruptible_timeout(queue, condition, timeout) 
	//   		is interruptible and the timeout is measured in jiffies
	
	if (wait_event_interruptible(scull_queue, *data)){
		return -ERESTARTSYS;
	}

	printk(KERN_DEBUG "arrriba, I am awoken\n");

	count = count > 1024 ? 1024 : count;
	if (copy_to_user(buf, data, count)){
		return -EFAULT;
	}
	
	*data = '\0';

	return count;
}

ssize_t scull_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	// modify "data" so the wait queue is triggered
	
	if (count > 1024)
		count = 1024;

	if (copy_from_user(data, buf, count)){
		return -EFAULT;
	}
	wake_up_interruptible(&scull_queue);

	return count; 
}

struct file_operations scull_fops = {
	.owner = THIS_MODULE,
	.read = scull_read,
	.write = scull_write,
};

int scull_init (void) 
{
	int err = 0;

	data = kmalloc(sizeof(char) * 1024, GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	*data = '\0';

	err = alloc_chrdev_region(&device_num, scull_minor, 1, "scull");
	if (err)
		return -1;

	scull_major = MAJOR(device_num);
	scull_minor = MINOR(device_num);

	scull_cdev = cdev_alloc();
	cdev_init(scull_cdev, &scull_fops);
	
	err = cdev_add(scull_cdev, device_num, 1);

	printk(KERN_DEBUG "Successfully loaded with Major: %d and Minor: %d\n", scull_major, scull_minor);

	// init the wait queue
	//init_waitqueue_head(&scull_queue);

	return 0;
}

void scull_exit(void)
{
	if (data)
		kfree(data);

	cdev_del(scull_cdev);
	unregister_chrdev_region(device_num, 1);

	return;
}

module_init(scull_init);
module_exit(scull_exit);


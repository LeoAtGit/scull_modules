#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/slab.h>
#include <linux/fs.h>

#include <linux/cdev.h>

#include <asm/uaccess.h>

#include <linux/wait.h>
#include <linux/sched.h>

int scull_major = 0;
int scull_minor = 0;

int device_num = 0;

typedef struct scull_pipe {
	char* rp;
	char* wp;
	struct wait_queue_head_t *iq;
	struct wait_queue_head_t *oq;

	char *buffer;

	struct semaphore sem;
	struct cdev *scull_cdev;
} scull_pipe;

scull_pipe *scull_pipe_device; 

ssize_t scull_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	return count;
}

ssize_t scull_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
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

	/*data = kmalloc(sizeof(char) * 1024, GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	*data = '\0';*/

	scull_pipe_device = kmalloc(sizeof(struct scull_pipe), GFP_KERNEL);
	if (!scull_pipe_device) {
		pr_debug("Couldn't malloc scull_pipe_device\n");
		return -ENOMEM;
	}

	err = alloc_chrdev_region(&device_num, scull_minor, 1, "scull");
	if (err)
		return -1;

	scull_major = MAJOR(device_num);
	scull_minor = MINOR(device_num);

	scull_pipe_device->scull_cdev = cdev_alloc();
	cdev_init(scull_pipe_device->scull_cdev, &scull_fops);
	
	err = cdev_add(scull_pipe_device->scull_cdev, device_num, 1);

	pr_debug("Successfully loaded with Major: %d and Minor: %d\n", scull_major, scull_minor);

	return 0;
}

void scull_exit(void)
{
	cdev_del(scull_pipe_device->scull_cdev);
	unregister_chrdev_region(device_num, 1);

	if (scull_pipe_device) 
		kfree(scull_pipe_device); /* Check if we need more kfree's */

	return;
}

module_init(scull_init);
module_exit(scull_exit);

MODULE_AUTHOR("Leo Sperling");
MODULE_LICENSE("GPL");

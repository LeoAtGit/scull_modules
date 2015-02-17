#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/slab.h>

#include "scull0.h"

static char *test_string = "This is the test string";

int scull_major = 0;
int scull_minor = 0;

int dev_num = 0; /* The number of the device */

struct scull_device *scull_device = NULL;
struct data_set *first_data_set = NULL;

struct file_operations scull_fops = {
	.owner = THIS_MODULE,
	.read = scull_read,
	.write = scull_write,
	.open = scull_open,
	.release = scull_release,
};

ssize_t scull_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	//TODO struct scull_device *dev = filp->private_data;
	struct scull_device *dev = scull_device;
	struct data_set *crawler = NULL;
	ssize_t ret = 0;

	printk(KERN_NOTICE "entered scull_read\n");
	
	crawler = dev->data;
	if (!crawler){ /* if this is NULL then read nothing */
		printk(KERN_ERR "The crawler in the read function is a null pointer\n");
		goto out;
	}

	if (count > dev->size)
		count = dev->size;

	/*TODO while(crawler->next_node){
		if (copy_to_user(buf, crawler->data, (count > crawler->size) ? crawler->size : count)){
			ret = -EFAULT;
			goto out;
		}

		crawler = crawler->next_node;
	}

	if (copy_to_user(buf, crawler->data, (count > crawler->size) ? crawler->size : count)){
		ret = -EFAULT;
		goto out;
	}*/
	if (copy_to_user(buf, crawler->data, (count > crawler->size) ? crawler->size : count)){
		ret = -EFAULT;
		goto out;
	}

	printk(KERN_NOTICE "crawler           :   %p\n", crawler);
	printk(KERN_NOTICE "crawler->data     :   %p\n", crawler->data);
	printk(KERN_NOTICE "crawler->prev_node:   %p\n", crawler->prev_node);
	printk(KERN_NOTICE "crawler->next_node:   %p\n", crawler->next_node);

	if (crawler->next_node)
		crawler = crawler->next_node;

	if (copy_to_user(buf, crawler->data, (count > crawler->size) ? crawler->size : count)){
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
	//TODO struct scull_device *dev = filp->private_data;
	struct scull_device *dev = scull_device;
	struct data_set *d_set = dev->data;
	struct data_set *new_data = NULL;
	void *buffer = NULL;
	ssize_t ret = 0;

	printk(KERN_NOTICE "dev:   %p\n", dev);
	printk(KERN_NOTICE "d_set: %p\n", d_set);

	printk(KERN_NOTICE "entered scull_write\n");

	if (d_set == NULL){
		// first time writing to the device
		/*d_set = kmalloc(sizeof(data_set), GFP_KERNEL);
		if (!d_set){
			printk(KERN_ERR "Couldnt alloc memory for the first data\n");
			ret = -ENOMEM;
			goto out;
		}*/
		printk(KERN_ERR "This shouldnt happen, the first data set is empty\n");
		ret = -EFAULT;
		goto out;
	}

	printk(KERN_NOTICE "line 65\n");

	while(d_set->next_node != NULL){
		buffer = &d_set;
		d_set = d_set->next_node;
	}

	printk(KERN_NOTICE "line 70\n");

	new_data = kmalloc(sizeof(struct data_set), GFP_KERNEL);
	if (!new_data){
		printk(KERN_ERR "Couldnt allocate space for the new data set\n");
		ret = -ENOMEM;
		goto out;
	}

	new_data->data = kmalloc(count * sizeof(char), GFP_KERNEL);
	if (!new_data->data){
		ret = -ENOMEM;
		goto out;
	}
	
	d_set->next_node = new_data;
	if (buffer){
		new_data->prev_node = d_set;
	}

	printk(KERN_NOTICE "line 83\n");

	memset(new_data->data, 0, count * sizeof(char));
	dev->size += count;

	printk(KERN_NOTICE "line 92\n");

	if (copy_from_user(new_data->data, buf, count)){
		ret = -EFAULT;
		goto out;
	}

	printk(KERN_NOTICE "line 99\n");

	new_data->size = count;

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
	
	err_code = cdev_add(&dev->cdev, dev_num, 1);
	if(err_code)
		printk(KERN_NOTICE "Error %d: Error adding scull0!\n", err_code);
}

void scull_cdev_del(struct scull_device *dev)
{
	printk(KERN_NOTICE "entered scull_cdev_del\n");
	printk(KERN_ERR "[DEBUG] we are now in scull_cdev_del\n");

	if (dev->data)
//TODO		delete_linked_list();

	printk(KERN_ERR "[DEBUG] I could free dev->data without a segfault\n");

	cdev_del(&dev->cdev);

	return;
}

void delete_linked_list()
{
	struct data_set *crawler = scull_device->data;
	void *buffer = NULL;

	printk(KERN_NOTICE "entered delete_linked_list");

	if (!crawler && scull_device->size){
		printk(KERN_ERR "ERROR: couldnt get the data pointer");
		return;
	}

	while(crawler->next_node){
		crawler = crawler->next_node;
	}

	while(crawler->prev_node){
		buffer = (void*) crawler->prev_node;
		if (crawler->data){
			kfree(crawler->data);
		}
		kfree(crawler);

		crawler = (struct data_set *) buffer;
	}
}

int scull_init(void)
{
	int err = 0;
	struct cdev *scull_cdev;
//	struct data_set *first_data_set;

	printk(KERN_NOTICE "entered scull_init\n");
	printk(KERN_DEBUG "Attempting to load the module\n");

	first_data_set = kmalloc(sizeof(struct data_set), GFP_KERNEL);
	if (!first_data_set)
		return -ENOMEM;
	first_data_set->data = test_string;
	first_data_set->size = strlen(test_string);
	first_data_set->prev_node = NULL;
	first_data_set->next_node = NULL;

	err = alloc_chrdev_region(&dev_num, scull_minor, 1, "scull");
	if (err)
		return -1;
	scull_major = MAJOR(dev_num);
	scull_minor = MINOR(dev_num);

	scull_device = kmalloc(sizeof(struct scull_device), GFP_KERNEL);
	if (!scull_device)
		return -ENOMEM;

	scull_cdev = cdev_alloc();
	scull_device->cdev = *scull_cdev;
	scull_device->data = first_data_set;
	scull_device->size = first_data_set->size;

	scull_cdev_init(scull_device);

	printk(KERN_ERR "LOADED\n");
	return err;
}

void scull_clean_up(void)
{
	printk(KERN_NOTICE "entered scull_clean_up\n");

//	delete_linked_list();

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


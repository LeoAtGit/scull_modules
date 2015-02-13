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

struct scull_device *scull_device = NULL;
//struct data_set *first_data_set = NULL;

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

	while(dev->data->next_node){
		if (copy_to_user(buf, dev->data->data, (count > dev->data->size) ? dev->data->size : count)){ 
			ret = -EFAULT;
			goto out;
		}
	}

	*f_pos += count; 
	ret = count;

out:
	return ret;
}

ssize_t scull_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	struct scull_device *dev = filp->private_data;
	struct data_set *d_set = dev->data;
	void *buffer = NULL;
	ssize_t ret = 0;

	printk(KERN_NOTICE "entered scull_write\n");

	if (!d_set){
		printk("WTF WHY IS THIS A FUKCING NULL POINTER OMFG\n");
		return -1;
	}

	printk(KERN_NOTICE "line 65\n");

	while(d_set->next_node != NULL){
		printk(KERN_NOTICE "line 67\n");
		buffer = (void *) d_set;
		printk(KERN_NOTICE "line 69\n");
		d_set = d_set->next_node;
		printk(KERN_NOTICE "line 71\n");
	}

	printk(KERN_NOTICE "line 70\n");

	/*if (d_set->data)
		kfree(d_set->data);
*/
	if (scull_device->data->data)
		kfree(scull_device->data->data);

	printk(KERN_NOTICE "line 75\n");

	d_set->data = kmalloc(count * sizeof(char), GFP_KERNEL);
	if (buffer){
		d_set->prev_node = (struct data_set *) buffer;
		d_set->prev_node->next_node = d_set;
	}

	printk(KERN_NOTICE "line 83\n");

	if (!d_set->data){
		ret = -ENOMEM;
		goto out;
	}
	memset(d_set->data, 0, count * sizeof(char));
	dev->size += count;

	printk(KERN_NOTICE "line 92\n");

	if (copy_from_user(d_set->data, buf, count)){
		ret = -EFAULT;
		goto out;
	}

	printk(KERN_NOTICE "line 99\n");

	d_set->size = count;

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
		delete_linked_list();

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
	struct data_set *first_data_set;

	printk(KERN_NOTICE "entered scull_init\n");
	printk(KERN_DEBUG "Attempting to load the module\n");

	first_data_set = kmalloc(sizeof(data_set), GFP_KERNEL);
	if (!first_data_set)
		return -ENOMEM;
	first_data_set->data = NULL;
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


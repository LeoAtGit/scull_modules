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

// main struct for the device
struct scull_device *scull_device = NULL;
// in this linked list everything is going to be saved
struct data_set *first_data_set = NULL;

// the file operations of the scull_device
struct file_operations scull_fops = {
	.owner = THIS_MODULE,
	.read = scull_read,
	.write = scull_write,
	.open = scull_open,
	.release = scull_release,
};

// gets called when there is a read system call on this device
ssize_t scull_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	struct scull_device *dev = filp->private_data;
	struct data_set *crawler = NULL;
	ssize_t ret = 0;
	int orig_count = count;
	int missing_count = count;
	char *collected_data = NULL;

	// crawler crawls throught the linked list and reads it into collected_data
	crawler = dev->data;
	
	// if this crawler is NULL or doesnt contain data then read nothing
	if (!crawler || (!crawler->data && !crawler->next_node)){ 
		printk(KERN_ERR "The crawler in the read function is a null pointer\n");
		goto out;
	}
	
	// adjust the count so no buffer overflow happens
	if (count > dev->size)
		count = dev->size;

	// in collected_data all the data will be stored
	collected_data = kmalloc(count * sizeof(char) + 1, GFP_KERNEL);
	if (!collected_data){
		ret = -ENOMEM;
		goto out;
	}
	memset(collected_data, '\0', count * sizeof(char) + 1);
	
	strcpy(collected_data, crawler->data);
	missing_count -= crawler->size;

	// go through the linked list and store the data in collected_data
	while(crawler->next_node){
		crawler = crawler->next_node;

		// check for buffer overflows
		if (crawler->size > missing_count){
			break;
		}
		else{
			strcat(collected_data, crawler->data); 
			missing_count -= crawler->size;
		}
	}

	// copy the data to the user
	if (copy_to_user(buf, collected_data, count)){
		ret = -EFAULT;
		goto out;
	}
	
	*f_pos += orig_count; 
	ret = orig_count;

out:
	return ret;
}

// gets called when there is a write system call on this device
ssize_t scull_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	struct scull_device *dev = filp->private_data;
	struct data_set *d_set = dev->data;
	struct data_set *new_data = NULL;
	ssize_t ret = 0;
	int flag_is_first = 0;

	// check whether it is the first time writing to this device
	if (d_set == NULL){
		flag_is_first = 1;
		d_set = kmalloc(sizeof(struct data_set), GFP_KERNEL);
		if (!d_set){
			printk(KERN_ERR "Couldnt alloc memory for the first data\n");
			ret = -ENOMEM;
			goto out;
		}
		d_set->data = NULL;
		d_set->next_node = NULL;
		d_set->prev_node = NULL;

		dev->data = d_set;
	}

	// crawl through the linked list
	while(d_set->next_node){
		d_set = d_set->next_node;
	}

	// allocate a new data set to which the data will be written to
	new_data = kmalloc(sizeof(struct data_set), GFP_KERNEL);
	if (!new_data){
		printk(KERN_ERR "Couldnt allocate space for the new data set\n");
		ret = -ENOMEM;
		goto out;
	}

	new_data->data = kmalloc(count * sizeof(char) + 1, GFP_KERNEL);
	if (!new_data->data){
		ret = -ENOMEM;
		goto out;
	}

	new_data->next_node = NULL;

	d_set->next_node = new_data;
	new_data->prev_node = d_set;

	memset(new_data->data, '\0', count * sizeof(char) + 1);
	dev->size += count;

	// copy the data from the user to the device
	if (copy_from_user(new_data->data, buf, count)){
		ret = -EFAULT;
		goto out;
	}

	new_data->size = count;

	// further initialization needed when it is the first data set in the device
	if (flag_is_first){
		d_set->data = new_data->data;
		d_set->prev_node = NULL;
		d_set->next_node = new_data->next_node;
	}

	dev->data->data = new_data->data;

	*f_pos += count;
	ret = count;

out:
	return ret;
}

// gets called when the open system call is called on this device
int scull_open(struct inode *inode, struct file *filp)
{
	struct scull_device *dev;

	// hacky solution i dont really understand
	// this macro is somewhere defined in the kernel sources
	dev = container_of(inode->i_cdev, struct scull_device, cdev);
	// for later use save the device in the filp
	// TODO find out what filp is :D
	filp->private_data = dev;

	return 0;
}

// gets called when we close the file descriptor of the device. Do nothing
int scull_release(struct inode *inode, struct file *filp)
{
	return 0;
}

// initialize the cdev structure
// This structure holds information about the device for the kernel
void scull_cdev_init(struct scull_device *dev)
{
	int err_code = 0;

	cdev_init(&dev->cdev, &scull_fops);
	dev->cdev.owner = THIS_MODULE;
	
	err_code = cdev_add(&dev->cdev, dev_num, 1);
	if(err_code)
		printk(KERN_NOTICE "Error %d: Error adding scull0!\n", err_code);
}

// delete the cdev structure
void scull_cdev_del(struct scull_device *dev)
{
	cdev_del(&dev->cdev);

	return;
}

// delete the linked list of data
void delete_linked_list()
{
	struct data_set *crawler = scull_device->data;
	void *buffer = NULL;

	if (!crawler && !scull_device->size){
		return;
	}

	// crawler crawls to the end of the linked list 
	while(crawler->next_node){
		crawler = crawler->next_node;
	}

	// crawler frees the linked list from behind
	while(crawler->prev_node){
		buffer = (void*) crawler->prev_node;
		if (crawler->data){
			kfree(crawler->data);
		}
		kfree(crawler);

		crawler = (struct data_set *) buffer;
	}
}

// gets called as the first function of the module
int scull_init(void)
{
	int err = 0;
	struct cdev *scull_cdev;

	// the data set which will act as an entry point
	first_data_set = kmalloc(sizeof(struct data_set), GFP_KERNEL);
	if (!first_data_set)
		return -ENOMEM;
	first_data_set->data = NULL;
	first_data_set->size = 0;
	first_data_set->prev_node = NULL;
	first_data_set->next_node = NULL;

	// allocate the dev numbers and stuff
	err = alloc_chrdev_region(&dev_num, scull_minor, 1, "scull");
	if (err)
		return -1;
	scull_major = MAJOR(dev_num);
	scull_minor = MINOR(dev_num);

	// the struct the whole module depends upon
	scull_device = kmalloc(sizeof(struct scull_device), GFP_KERNEL);
	if (!scull_device)
		return -ENOMEM;

	// some initializations
	scull_cdev = cdev_alloc();
	scull_device->cdev = *scull_cdev;
	scull_device->data = first_data_set;
	scull_device->size = first_data_set->size;

	scull_cdev_init(scull_device);

	printk(KERN_ERR "LOADED\n");
	return err;
}

// clean up function, exit function
void scull_clean_up(void)
{
	// delete all of the stuff i allocated in the linked list
	delete_linked_list();

	// delete the cdev structure 
	// and tell the kernel that i dont need the dev struct anymore
	scull_cdev_del(scull_device);

	// one last free
	if (scull_device)
		kfree(scull_device);

	// free the dev nums and such for other programs
	unregister_chrdev_region(dev_num, 1);

	printk(KERN_ERR "UNLOADED\n");
}

MODULE_AUTHOR("Leo Sperling");
MODULE_LICENSE("GPL");

module_init(scull_init);
module_exit(scull_clean_up);


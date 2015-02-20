#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>

// this structure holds the data and is implemented as a linked list
typedef struct data_set{
	void *data;			/* the actual data, could be a string */
	unsigned long size;		/* the size of "data" */
	struct data_set *prev_node;	/* linked list previous node */
	struct data_set *next_node;	/* linked list next node */
} data_set;

// this structure represents the device and all of its functions (saving a string)
struct scull_device {
	struct data_set *data;	/* Pointer to the data which we got from userspace*/
	unsigned long size;	/* Amount of data stored in here */
	struct cdev cdev;	/* Char device structure for the kernel */
};

// initializes the cdev structure for the kernel
static void scull_cdev_init(struct scull_device *);

// removes the cdev structure for the kernel
static void scull_cdev_del(struct scull_device *);

// gets called as the first function and initializes a bunch of stuff
int scull_init(void);

// gets called as the last function and deallocates and stuff
void scull_clean_up(void);

// part of fops
ssize_t scull_read(struct file *, char *, size_t count, loff_t *);

// part of fops
ssize_t scull_write(struct file *, const char *, size_t count, loff_t *);

// part of fops
int scull_open(struct inode *, struct file *);

// part of fops
int scull_release(struct inode *, struct file *);

// deletes and frees the linked list
void delete_linked_list(void);


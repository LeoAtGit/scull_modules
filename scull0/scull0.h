#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>

typedef struct data_set{
	void *data;
	unsigned long size;
	struct data_set *prev_node;
	struct data_set *next_node;
} data_set;

struct scull_device {
	struct data_set *data;	/* Pointer to the data which we got from userspace*/
	unsigned long size; /* Amount of data stored in here */
	struct cdev cdev;   /* Char device structure */
};

static void scull_cdev_init(struct scull_device *);

static void scull_cdev_del(struct scull_device *);

int scull_init(void);

void scull_clean_up(void);

ssize_t scull_read(struct file *, char *, size_t count, loff_t *);

ssize_t scull_write(struct file *, const char *, size_t count, loff_t *);

int scull_open(struct inode *, struct file *);

int scull_release(struct inode *, struct file *);

void delete_linked_list(void);


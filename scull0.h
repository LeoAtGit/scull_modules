#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>

struct scull_device {
	void *data;	    /* Pointer to some abitrary number */
	//unsigned long size; /* Amount of data stored in here */
	struct cdev cdev;   /* Char device structure */
};

static void scull_cdev_init(struct scull_device *);

static void scull_cdev_del(struct scull_device *);

int scull_init(void);

void scull_clean_up(void);


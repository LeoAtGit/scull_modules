#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/slab.h>
#include <linux/fs.h>

#include <linux/cdev.h>

#include <asm/uaccess.h>
#include "scull.h"

/* scull_read
 * Invoked when a read comes in. It will look if there is data to be read 
 * and if not it will go to sleep. When data is available it will wake up and
 * print to userspace the data
 */
ssize_t scull_read(struct file *filp, 
		   char *buf, 
		   size_t count, 
		   loff_t *f_pos)
{
	size_t max_read = 0;

	while (count != 0) {
		/* get the semaphore to check and change the pointers */
		while (down_interruptible(&scull->sem)) {
			pr_debug("Couldn't get the semaphore, so we go to sleep\n");
			wait_event_interruptible(oq, !(down_interruptible(&scull->sem)));
		}
		
		/* Wrap around */
		if (scull->rp == scull->buf_end
	        		&& scull->wp 
					!= scull->buf_end) {
			scull->rp = scull->buffer;
		}
		
		max_read = get_max_read();
		
		if (max_read) {
			if (copy_to_user(buf,
					 scull->rp,
					 max_read)) {
				pr_debug("Couldn't copy to user\n");
				return -1;
			}

			scull->rp += max_read;
			count -= max_read;
			up(&scull->sem);
		} else {
			up(&scull->sem);
			/* go to sleep */
			wait_event_interruptible(oq, get_max_read());
			pr_debug("I woke up!");
		}

		pr_debug("buffer = %p\n", scull->buffer);
		pr_debug("rp new = %p\n", scull->rp);
		pr_debug("wp new = %p\n", scull->wp);
		pr_debug("end    = %p\n\n", scull->buf_end);
	}

	return count;
}

/* scull_write
 * Will only write if there is space free in the buffer. If there is no space,
 * go to sleep and wait until the scull_read function reads from the buffer,
 * thus changing the scull_pipe.rp and there is new place to write to.
 */
ssize_t scull_write(struct file *filp, 
		    const char *buf, 
		    size_t count, 
		    loff_t *f_pos)
{
	size_t max_count = 0;
	size_t bytes_written = 0;
	
	/* debug */
	//wake_up_interruptible(&oq);

	while (count != 0) {
		/* get the semaphore so you can make critical checks and 
		 * so you can write to the buffer without any risk.
		 */
		while (down_interruptible(&scull->sem)) {
			pr_debug("Couldn't get the semaphore, so we go to sleep\n");
			/*FIXME */
			return -ERESTARTSYS;
		}
		
		max_count = get_max_write();
		pr_debug("max_count: %li", max_count);

		if (max_count) {
			if (max_count > count) {
				max_count = count;
			}

			if (copy_from_user(scull->wp,
					   buf,
					   max_count)) {
				pr_debug("Couldn't write to the buffer.\n");
				return -1;
			}

			count -= max_count;
			bytes_written += max_count;
			scull->wp += max_count;
			
			up(&scull->sem);
		} else {
			/* release the semaphore, so other processes can empty
			 * the buffer while you sleep so that you can write to 
			 * it again
			 */
			up(&scull->sem);
			/* go to sleep */
			pr_debug("sleeping\n"); /* FIXME */
			wait_event_interruptible(iq, get_max_write());
		}
		pr_debug("buffer = %p\n", scull->buffer);
		pr_debug("rp new = %p\n", scull->rp);
		pr_debug("wp new = %p\n", scull->wp);
		pr_debug("end    = %p\n\n", scull->buf_end);
	}

	return bytes_written; 
}

size_t get_max_read(void) 
{
	char *wp, *rp, *end;
	
	wp = scull->wp;
	rp = scull->rp;
	end = scull->buf_end;

	if (wp == rp) {
		return 0;
	} else if (wp > rp) {
		return wp - rp;
	} else if (rp > wp) {
		return end - rp;
	} else {
		return 0; /* Never happens, but compiler complains otherwise */
	}
}

size_t get_max_write(void)
{
	char *wp, *rp, *end;
	
	wp = scull->wp;
	rp = scull->rp;
	end = scull->buf_end;

	if (wp == end && rp == scull->buffer) {
		return 0;
	} else if (wp == end && rp != scull->buffer) {
		scull->wp = scull->buffer;
		wp = scull->wp;
	}

	if (wp >= rp) {
		return end - wp;
	} else {
		if (rp - wp == 1) /* Buffer full */
			return 0;
		return (rp - 1) - wp;
	}
}

int scull_init (void) 
{
	int err = 0;

	scull = kmalloc(sizeof(scull_pipe), GFP_KERNEL);
	if (!scull) {
		pr_debug("Couldn't malloc scull\n");
		return -ENOMEM;
	}
	scull->rp = scull->buffer;
	scull->wp = scull->buffer;
	scull->buf_end = scull->buffer + BUF_SIZE;
	sema_init(&scull->sem, 1);

	err = alloc_chrdev_region(&device_num, scull_minor, 1, "scull");
	if (err) {
		pr_debug("Couldn't allocate memory for the char device\n");
		goto free;
	}

	scull_major = MAJOR(device_num);
	scull_minor = MINOR(device_num);

	scull->scull_cdev = cdev_alloc();
	cdev_init(scull->scull_cdev, &scull_fops);
	
	err = cdev_add(scull->scull_cdev, device_num, 1);

	pr_debug("Successfully loaded with Major: %d and Minor: %d\n", scull_major, scull_minor);

	return 0;

free:
	if (scull)
		kfree (scull);
	return -1;
}

void scull_exit(void)
{
	cdev_del(scull->scull_cdev);
	unregister_chrdev_region(device_num, 1);

	if (scull) 
		kfree(scull); /* Check if we need more kfree's */
}

module_init(scull_init);
module_exit(scull_exit);

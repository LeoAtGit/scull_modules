#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/slab.h>
#include <linux/fs.h>

#include <linux/cdev.h>

#include <asm/uaccess.h>

#include <linux/wait.h>
#include <linux/sched.h>

#include <linux/semaphore.h>

#define BUF_SIZE 10 /* Buffer size of the circular buffer */

int scull_major = 0;
int scull_minor = 0;

int device_num = 0;
DECLARE_WAIT_QUEUE_HEAD(iq);

/* struct scull_pipe
 * Is the main struct here, I/O will be read/written to the scull_pipe.buffer
 * and the rp and wp will say where to read/write from/to.
 */
typedef struct scull_pipe {
	char *rp, *wp;			/* The read/write pointer shows where  
       					 * the buffer the next read/write will
					 * be */
	/* struct wait_queue_head_t oq; */   /* The wait queue for input and 
					 * output */

	char buffer[BUF_SIZE];		/* Buffer is circular */
	char *buf_end;			/* The end address of the buffer */

	struct semaphore sem;		/* Needed for checking if there is data
					 * to be read or not */
	struct cdev *scull_cdev;	/* Character device for /dev/ */
} scull_pipe;

scull_pipe *scull_pipe_device; /* Main data structure in this driver */

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
	char *wp, *rp, *end;
	size_t max_read = 0;

	while (count != 0) {
		/* get the semaphore to check and change the pointers */
		while (down_interruptible(&scull_pipe_device->sem)) {
			pr_debug("Couldn't get the semaphore, so we go to sleep\n");
			/*FIXME*/
			return -ERESTARTSYS;
		}
		
		if (scull_pipe_device->rp == scull_pipe_device->buf_end
	        		&& scull_pipe_device->wp 
					!= scull_pipe_device->buf_end) {
			scull_pipe_device->rp = scull_pipe_device->buffer;
		}
		
		wp = scull_pipe_device->wp;
		rp = scull_pipe_device->rp;
		end = scull_pipe_device->buf_end;

		if (wp == rp) {
			max_read = 0;
		} else if (wp > rp) {
			max_read = wp - rp;
		} else if (rp > wp) {
			max_read = end - rp;
		}
		
		pr_debug("max_read before: %li\n", max_read);
		if  (max_read > count) {
			max_read = count;
		}
		pr_debug("max_read after: %li\n", max_read);

		if (max_read) {
			if (copy_to_user(buf,
					 scull_pipe_device->rp,
					 max_read)) {
				pr_debug("Couldn't copy to user\n");
				return -1;
			}

			scull_pipe_device->rp += max_read;
			count -= max_read;
			up(&scull_pipe_device->sem);
		} else {
			up(&scull_pipe_device->sem);
			count = 0;
		}

		pr_debug("buffer = %p\n", scull_pipe_device->buffer);
		pr_debug("rp new = %p\n", scull_pipe_device->rp);
		pr_debug("wp new = %p\n", scull_pipe_device->wp);
		pr_debug("end    = %p\n\n", scull_pipe_device->buf_end);
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
	char *wp, *rp, *end;
	size_t max_count = 0;
	size_t bytes_written = 0;

	while (count != 0) {
		/* get the semaphore so you can make critical checks and 
		 * so you can write to the buffer without any risk.
		 */
		while (down_interruptible(&scull_pipe_device->sem)) {
			pr_debug("Couldn't get the semaphore, so we go to sleep\n");
			/*FIXME */
			return -ERESTARTSYS;
		}

		wp = scull_pipe_device->wp;
		rp = scull_pipe_device->rp;
		end = scull_pipe_device->buf_end;

		if (wp == end && rp == scull_pipe_device->buffer) {
			max_count = 0;
			goto __max_count;
		} else if (wp == end && rp != scull_pipe_device->buffer) {
			scull_pipe_device->wp = scull_pipe_device->buffer;
			wp = scull_pipe_device->wp;
		}

		if (wp >= rp) {
			max_count = end - wp;
		} else {
			if (rp - wp == 1) /* Buffer full */
				max_count = 0;
			max_count = (rp - 1) - wp;
		}
__max_count:
		if (max_count) {
			if (max_count > count) {
				max_count = count;
			}

			if (copy_from_user(scull_pipe_device->wp,
					   buf,
					   max_count)) {
				pr_debug("Couldn't write to the buffer.\n");
				return -1;
			}

			count -= max_count;
			bytes_written += max_count;
			scull_pipe_device->wp += max_count;
			
			up(&scull_pipe_device->sem);
		} else {
			/* release the semaphore, so other processes can empty
			 * the buffer while you sleep so that you can write to 
			 * it again
			 */
			up(&scull_pipe_device->sem);
			/* go to sleep */
			pr_debug("sleeping\n");
			wait_event_interruptible(iq, true);
		}
		pr_debug("buffer = %p\n", scull_pipe_device->buffer);
		pr_debug("rp new = %p\n", scull_pipe_device->rp);
		pr_debug("wp new = %p\n", scull_pipe_device->wp);
		pr_debug("end    = %p\n\n", scull_pipe_device->buf_end);
	}

	return bytes_written; 
}

struct file_operations scull_fops = {
	.owner = THIS_MODULE,
	.read = scull_read,
	.write = scull_write,
};

int scull_init (void) 
{
	int err = 0;

	scull_pipe_device = kmalloc(sizeof(scull_pipe), GFP_KERNEL);
	if (!scull_pipe_device) {
		pr_debug("Couldn't malloc scull_pipe_device\n");
		return -ENOMEM;
	}
	scull_pipe_device->rp = scull_pipe_device->buffer;
	scull_pipe_device->wp = scull_pipe_device->buffer;
	scull_pipe_device->buf_end = scull_pipe_device->buffer + BUF_SIZE;
	sema_init(&scull_pipe_device->sem, 1);

	err = alloc_chrdev_region(&device_num, scull_minor, 1, "scull");
	if (err) {
		pr_debug("Couldn't allocate memory for the char device\n");
		goto free;
	}

	scull_major = MAJOR(device_num);
	scull_minor = MINOR(device_num);

	scull_pipe_device->scull_cdev = cdev_alloc();
	cdev_init(scull_pipe_device->scull_cdev, &scull_fops);
	
	err = cdev_add(scull_pipe_device->scull_cdev, device_num, 1);

	//init_waitqueue_head(&scull_pipe_device->iq);
//	init_waitqueue_head(&scull_pipe_device->oq);

	pr_debug("Successfully loaded with Major: %d and Minor: %d\n", scull_major, scull_minor);

	return 0;

free:
	if (scull_pipe_device)
		kfree (scull_pipe_device);
	return -1;
}

void scull_exit(void)
{
	cdev_del(scull_pipe_device->scull_cdev);
	unregister_chrdev_region(device_num, 1);

	if (scull_pipe_device) 
		kfree(scull_pipe_device); /* Check if we need more kfree's */
}

module_init(scull_init);
module_exit(scull_exit);

MODULE_AUTHOR("Leo Sperling");
MODULE_LICENSE("GPL");

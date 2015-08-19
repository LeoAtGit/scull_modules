#ifndef SCULL_H
#define	SCULL_H

#include <linux/semaphore.h>                /* For the semaphore in the
                                               scull_pipe */
#include <linux/cdev.h>                     /* For the cdev stuff */

#include <linux/wait.h>                     /* For waitqueue stuff */
#include <linux/sched.h>                    /* For waitqueue stuff */

#define BUF_SIZE 10 /* Buffer size of the circular buffer */

int scull_major = 0;
int scull_minor = 0;
int device_num = 0;

static DECLARE_WAIT_QUEUE_HEAD(iq);
static DECLARE_WAIT_QUEUE_HEAD(oq);

/* struct scull_pipe
 * Is the main struct here, I/O will be read/written to the scull_pipe.buffer
 * and the rp and wp will say where to read/write from/to.
 */
typedef struct scull_pipe {
	char *rp, *wp;			/* The read/write pointer shows where  
       					 * the buffer the next read/write will
					 * be */
	char buffer[BUF_SIZE];		/* Buffer is circular */
	char *buf_end;			/* The end address of the buffer */

	struct semaphore sem;		/* Needed for checking if there is data
					 * to be read or not */
	struct cdev *scull_cdev;	/* Character device for /dev/ */
} scull_pipe;

scull_pipe *scull; /* Main data structure in this driver */

/* functions for all of the drivers duties */
ssize_t scull_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
ssize_t scull_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
int scull_init (void);
void scull_exit(void);
size_t get_max_read(void);
size_t get_max_write(void);

/* The fops */
struct file_operations scull_fops = {
	.owner = THIS_MODULE,
	.read = scull_read,
	.write = scull_write,
};

/* misc stuff */
MODULE_AUTHOR("Leo Sperling");
MODULE_LICENSE("GPL");

#endif	/* SCULL_H */
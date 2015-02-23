#include <linux/kernel.h>
#include <linux/module.h>

#undef PDEBUG
#ifdef SCULL_DEBUG
	/* When debugging is turned on, use this macro to write to dmesg */
#	define PDEBUG(msg, args...) printk(KERN_DEBUG "scull: " msg, ## args)
#else
#	define PDEBUG(msg, args...) /* When not debugging, print nothing */
#endif

#undef PPDEBUG
#define PPDEBUG(msg, args...) /* Nothing, just a placeholder */

#define QUANTUM_SIZE 64
#define QUANTUM_SET_SIZE 4

struct scull_data 
{
	char **data;
	struct scull_data *next_node;
};

struct scull_device
{
	struct scull_data *data;
	int quantum_size;
	int quantum_set_size;
	size_t size;
	struct cdev *cdev;
};

int scull_init(void);

ssize_t scull_read(struct file *, char *, size_t, loff_t *);
ssize_t scull_write(struct file *, const char *, size_t, loff_t *);
int scull_open(struct inode *, struct file *);
int scull_release(struct inode *, struct file *);


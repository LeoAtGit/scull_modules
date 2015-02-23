#include <linux/module.h>
#include <linux/kernel.h>

#include "scull.h"

int scull_init(void)
{
	PDEBUG("Loaded\n");

	return 0;
}

void scull_exit(void)
{
	PDEBUG("Unloaded\n");

	return;
}

module_init(scull_init);
module_exit(scull_exit);

MODULE_AUTHOR("Leo Sperling");
MODULE_LICENSE("GPL");


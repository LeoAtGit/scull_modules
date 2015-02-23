#undef PDEBUG
#ifdef SCULL_DEBUG
	/* When debugging is turned on, use this macro to write to dmesg */
#	define PDEBUG(msg, args...) printk(KERN_DEBUG "scull: " msg, ## args)
#else
#	define PDEBUG(msg, args...) /* When not debugging, print nothing */
#endif

#undef PPDEBUG
#define PPDEBUG(msg, args...) /* Nothing, just a placeholder */


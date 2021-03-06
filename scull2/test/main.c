#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#undef PDEBUG
#ifdef SCULL_DEBUG
	/* When debugging is turned on, use this macro to write to dmesg */
#	define PDEBUG(msg, args...) printk(KERN_DEBUG "scull: " msg, ## args)
#else
#	define PDEBUG(msg, args...) /* When not debugging, print nothing */
#endif

#undef PPDEBUG
#define PPDEBUG(msg, args...) /* Nothing, just a placeholder */

// Use 'k' as the magic number
#define SCULL_IOC_MAGIC 'k'

/*
 * S means SET through a ptr, 
 * T means TELL through a value,
 * G means GET: get the pointer,
 * Q means QUERY: get the value (return value)
 * X means EXCHANGE: switch G and S atomically
 * H means SHIFT: switch T and Q atomically
 * */
#define SCULL_IOCSDATA _IOW(SCULL_IOC_MAGIC, 1, int)
#define SCULL_IOCTDATA _IO(SCULL_IOC_MAGIC, 2)
#define SCULL_IOCGDATA _IOR(SCULL_IOC_MAGIC, 3, int)
#define SCULL_IOCQDATA _IO(SCULL_IOC_MAGIC, 4)
#define SCULL_IOCXDATA _IOWR(SCULL_IOC_MAGIC, 5, int)
#define SCULL_IOCHDATA _IO(SCULL_IOC_MAGIC, 6)

#define SCULL_IOC_MAXNR 6

#define SCULL_IOCBROKEN _IO('l', 7)

int main(int argc, char *argv[])
{
	int fd, ret;

	int test = 22;

	fd = open("/dev/scull", O_RDONLY);
	if (fd == -1){
		printf("Couldnt open the device!\n");
		return -1;
	}

	ret = ioctl(fd, SCULL_IOCSDATA, &test);
	if (ret){
		printf("Error code: %d", ret);
		ret = 0;
	}

	ret = ioctl(fd, SCULL_IOCBROKEN, &test);
	if (ret){
		printf("Error code: %d\n", ret);
		ret = 0;
	}

	test = 44;

	ret = ioctl(fd, SCULL_IOCTDATA, test);
	if (ret){
		printf("Error code: %d\n", ret);
		ret = 0;
	}

	ret = ioctl(fd, SCULL_IOCGDATA, &test);
	if (ret){
		printf("Error code: %d\n", ret);
		ret = 0;
	}
	printf("address of test: %p\n", &test);
	printf("value of test  : %d\n", test);

	test = 0;

	ret = ioctl(fd, SCULL_IOCQDATA);
	if (ret){
		printf("value returned: %d\n", ret);
		ret = 0;
	}

	test = 66;

	ret = ioctl(fd, SCULL_IOCXDATA, &test);
	if (ret){
		printf("Error code: %d\n", ret);
		ret = 0;
	}
	printf("address of test: %p\n", &test);
	printf("value of test  : %d\n", test);

	test = 88;

	ret = ioctl(fd, SCULL_IOCHDATA, test);
	if (ret){
		printf("Error code: %d\n", ret);
		ret = 0;
	}
	printf("address of test: %p\n", &test);
	printf("value of test  : %d\n", test);

	close(fd);

	return 0;
}


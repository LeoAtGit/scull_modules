obj-m += scull.o

# Change that line to enable/disable debugging
DEBUG = y

# Change the CFLAGS in respect of the DEBUG flag
ifeq ($(DEBUG), y)
	DEBFLAGS = -O -g -DSCULL_DEBUG # "-O" is needed to expand inlines +
				       # TODO find out what the -O option means
else
	DEBFLAGS = -O2
endif

ccflags-y += $(DEBFLAGS)

all:
	make -C /lib/modules/`uname -r`/build M=$(PWD) modules

clean:
	make -C /lib/modules/`uname -r`/build M=$(PWD) clean


/* Provides functions for read/write/ctl of char devices */
#include <kernel.h>
#include <memory.h>
#include <task.h>
#include <char.h>
#include <console.h>


int zero_rw(int rw, int m, char *buf, int c)
{
	m=0;
	if(rw == READ)
		memset(buf, 0, c);
	else if(rw == WRITE)
		return 0;
	return c;
}

int null_rw(int rw, int m, char *buf, int c)
{
	if((m=rw) == READ)
		return 0;
	return c;
}

/* Built in devices:
	0 -> Null
	1 -> Zero
	2 -> Mem
	3 -> ttyx
	4 -> tty
	5 -> serial
*/

chardevice_t *set_chardevice(int maj, int (*f)(int, int, char*, int), int (*c)(int, int, int), int (*s)(int, int))
{
	printk(1, "[dev]: Setting char device %d (%x, %x)\n", maj, f, c);
	chardevice_t *dev = (chardevice_t *)kmalloc(sizeof(chardevice_t));
	dev->func = f;
	dev->ioctl=c;
	dev->select=s;
	add_device(DT_CHAR, maj, dev);
	return dev;
}

int set_availablecd(int (*f)(int, int, char*, int), int (*c)(int, int, int), int (*s)(int, int))
{
	int i=10;
	while(i>0) {
		if(!get_device(DT_CHAR, i))
		{
			set_chardevice(i, f, c, s);
			break;
		}
		i++;
	}
	if(i < 0)
		return -EINVAL;
	return i;
}

void init_char_devs()
{
	/* These devices are all built into the kernel. We must initialize them now */
	set_chardevice(0, null_rw, 0, 0);
	set_chardevice(1, zero_rw, 0, 0);
	set_chardevice(3, ttyx_rw, ttyx_ioctl, ttyx_select);
	set_chardevice(4, tty_rw, tty_ioctl, tty_select);
	set_chardevice(5, serial_rw, 0, 0);
}

int char_rw(int rw, int dev, char *buf, int len)
{
	device_t *dt = get_device(DT_CHAR, MAJOR(dev));
	if(!dt)
		return -ENXIO;
	chardevice_t *cd = (chardevice_t *)dt->ptr;
	if(cd->func)
	{
		int ret = (cd->func)(rw, MINOR(dev), buf, len);
		return ret;
	}
	return 0;
}

void unregister_char_device(int n)
{
	printk(1, "[dev]: Unregistering char device %d\n", n);
	device_t *dev = get_device(DT_CHAR, n);
	if(!dev) return;
	void *fr = dev->ptr;
	dev->ptr=0;
	remove_device(DT_CHAR, n);
	kfree(fr);
}

int char_ioctl(int dev, int cmd, int arg)
{
	device_t *dt = get_device(DT_CHAR, MAJOR(dev));
	if(!dt)
		return -ENXIO;
	chardevice_t *cd = (chardevice_t *)dt->ptr;
	if(cd->ioctl)
	{
		int ret = (cd->ioctl)(MINOR(dev), cmd, arg);
		return ret;
	}
	return 0;
}

int chardev_select(struct inode *in, int rw)
{
	int dev = in->dev;
	device_t *dt = get_device(DT_CHAR, MAJOR(dev));
	if(!dt)
		return 1;
	chardevice_t *cd = (chardevice_t *)dt->ptr;
	if(cd->select)
		return cd->select(MINOR(dev), rw);
	return 1;
}

void send_sync_char()
{
	int i=0;
	while(i>=0) {
		device_t *d = get_n_device(DT_CHAR, i);
		if(!d) break;
		assert(d->ptr);
		chardevice_t *cd = (chardevice_t *)d->ptr;
		if(cd->ioctl)
			cd->ioctl(0, -1, 0);
		i++;
	}
}

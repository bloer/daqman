
/*
 * CAEN V1718 USB/VME Bridge driver - 0.5
 *
 * Written by Stefano Coluccini (s.coluccini@caen.it) - CAEN SpA
 *
 * based on rio500.c by Cesar Miquel (miquel@df.uba.ar)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 *
 *   TODO
 *   - Precise identification of the device based on strings.
 *   
 * Changelog:
 *
 *   March 2007 rev 0.5
 *     - Port to 2.6.18 kernel: dev_fs no more supported
 *   September 2006 rev 0.4
 *     - Port to 2.6.15 kernel: struct usb_driver::owner field is no more supported
 *     - Port to 64-bit architectures : no changes (tested on 2.6.15-1.2054_FC5 SMP x86_64)
 *     - Unlocked ioctl() method alleviates system performacne problems ( Linux Version>= 2.6.11)
 *     - BugFix: disconnect_v1718 tried to down the same lock twice ( Linux Version >= 2.5.0 )
 *   May 2006  rev 0.3
 *     - Porting to 2.6.15 kernel: usb_class_driver::mode field is no more supported
 *   January 2006  rev 0.2
 *     - Porting to 2.6 kernel
 *     - V1718_IOCTL_REV: get driver revision
 *     - !!! WARNING !!! conficts with usbtest driver. Remove it!
 */

#undef DEBUG  
 
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/random.h>
#include <linux/poll.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/usb.h>
#include <linux/smp_lock.h>
#ifdef CONFIG_DEVFS_FS
#include <linux/devfs_fs_kernel.h>
#endif

/*
 * Version Information
 */
#define DRIVER_VERSION "v0.5"
#define DRIVER_AUTHOR "Stefano Coluccini <s.coluccini@caen.it>"
#define DRIVER_DESC "CAEN V1718 USB/VME Bridge driver"

#define V1718_MINOR_BASE   178

/* stall/wait timeout for V1718 */
#define NAK_TIMEOUT (HZ)

#define IBUF_SIZE 0x10000	/* 64 kbytes */

/* Size of the V1718 buffer */
#define OBUF_SIZE 0x10000	/* 64 kbytes */

/* we can have up to this number of devices plugged in at once */
#define MAX_DEVICES	16
//
// Rev 0.2: added ini
#include <linux/version.h>

#ifndef VERSION
	#define VERSION(ver,rel,seq) (((ver)<<16) | ((rel)<<8) | (seq))
#endif	

#if LINUX_VERSION_CODE >= VERSION(2,5,0)
	
	#define USB_ST_TIMEOUT			(-ETIMEDOUT)
	#define USB_ST_DATAUNDERRUN		(-EREMOTEIO)
#endif

// Rev. 0.4
#if LINUX_VERSION_CODE >= VERSION(2,6,11)
        #include <linux/mutex.h>		// for unlocked ioctl interface
#endif

#include "v1718.h"
//
// Rev 0.2: added end

struct v1718_usb_data {
    struct usb_device *v1718_dev;   /* init: probe_v1718 */
#if LINUX_VERSION_CODE < VERSION(2,5,0)
    devfs_handle_t devfs;           /* devfs device */
#endif		
    unsigned int ifnum;             /* Interface number of the USB device */
    int open_count;                 /* number of users */
    int present;                    /* Device is present on the bus */
    int minor;                      /* minor number for the device */
    char *obuf, *ibuf;              /* transfer buffers */
    char bulk_in_ep, bulk_out_ep;   /* Endpoint assignments */
    wait_queue_head_t wait_q;       /* for timeouts */
    struct semaphore lock;          /* general race avoidance */
// Rev 0.4
#if LINUX_VERSION_CODE >= VERSION(2,6,11)
    struct mutex           ioctl_lock;
#endif
};

#if LINUX_VERSION_CODE < VERSION(2,5,0)
	extern devfs_handle_t usb_devfs_handle;	/* /dev/usb dir. */
#endif

/* array of pointers to our devices that are currently connected */
static struct v1718_usb_data *minor_table[MAX_DEVICES];

/* lock to protect the minor_table structure */
static DECLARE_MUTEX (minor_table_mutex);
//
//Rev 0.2
// static v1718_delete(struct v1718_usb_data *v1718)
static void v1718_delete(struct v1718_usb_data *v1718)
{
	minor_table[v1718->minor] = NULL;
	if( v1718->ibuf != NULL )
		kfree(v1718->ibuf);
	if( v1718->obuf != NULL )
		kfree(v1718->obuf);
	kfree(v1718);
}

// // Rev. 0.4
#if LINUX_VERSION_CODE >= VERSION(2,6,11)
static long unlocked_ioctl_v1718(struct file *, unsigned int, unsigned long);
#endif

static int open_v1718(struct inode *inode, struct file *file)
{
	struct v1718_usb_data *v1718 = NULL;
	int subminor;
//Dbg
//info( "v1718 opening...\n");


	subminor = MINOR(inode->i_rdev) - V1718_MINOR_BASE;
	if( (subminor < 0) || (subminor > MAX_DEVICES) )
		return -ENODEV;

	down(&minor_table_mutex);

	v1718 = minor_table[subminor];
	if( v1718 == NULL ) {
		up(&minor_table_mutex);
		return -ENODEV;
	}

	down(&v1718->lock);

	up(&minor_table_mutex);

	v1718->open_count++;

	init_waitqueue_head(&v1718->wait_q);

// Rev. 1.6
#if LINUX_VERSION_CODE >= VERSION(2,6,11)
        mutex_init(&v1718->ioctl_lock);
#endif
	file->private_data = v1718;

	up(&v1718->lock);

//	info("v1718 opened.");

//Dbg
//info( "v1718 opened...\n");

	return 0;
}

static int close_v1718(struct inode *inode, struct file *file)
{
	struct v1718_usb_data *v1718 = (struct v1718_usb_data *)file->private_data;
	int retval = 0;
	if( v1718 == NULL ) {
		err("Close: object is null");
		return -ENODEV;
	}
	down(&minor_table_mutex);
	down(&v1718->lock);

	if( v1718->open_count <= 0 ) {
		err("Device not opened");
		retval = -ENODEV;
		goto exit_not_opened;
	}

	if( v1718->v1718_dev == NULL ) {
		/* the device was unplugged before the file was released */
		up(&v1718->lock);
		v1718_delete(v1718);
		up(&minor_table_mutex);
		return 0;
	}

	v1718->open_count--;

exit_not_opened:
	up(&(v1718->lock));
	up(&minor_table_mutex);

//	info("v1718 closed.");
//Dbg
//info( "v1718 close...\n");

	return retval;
}

static int
ioctl_v1718(struct inode *inode, struct file *file, unsigned int cmd,
            unsigned long arg)
{
//
// rev 0.2
	int ret= 0;
	struct v1718_usb_data *v1718;
	
	v1718 = (struct v1718_usb_data *)file->private_data;
	down(&(v1718->lock));
    /* Sanity check to make sure v1718 is connected, powered, etc */
    if ( v1718 == NULL ||
         v1718->present == 0 ||
         v1718->v1718_dev == NULL )
	{
		up(&(v1718->lock));
		return -ENOTTY;
	}

    switch (cmd) {
        case V1718_IOCTL_REV:
			{
				v1718_rev_t rev;
                if( copy_from_user(&rev, (v1718_rev_t *)arg, sizeof(rev)) > 0 ) {
                        ret = -EFAULT;
                        break;
                }
				strcpy( rev.rev_buf, DRIVER_VERSION);
                if( copy_to_user((v1718_rev_t *)arg, &rev, sizeof(rev)) > 0) {
                        ret = -EFAULT;
                        break;
                }
			}
	        break;
		default:
			ret= -ENOTTY;
			break;
    }
	up(&(v1718->lock));
    return ret;
}

// Rev 0.4
/*
        ----------------------------------------------------------------------

        unlocked_ioctl_v1718 (Called in preference to ioctl_v1718 on newer kernels)

        ----------------------------------------------------------------------
*/

#if LINUX_VERSION_CODE >= VERSION(2,6,11)
static long unlocked_ioctl_v1718(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct inode *inode = file->f_dentry->d_inode;
        struct v1718_usb_data *s = (struct v1718_usb_data *)file->private_data;
	long ret;

	/* ioctl() calls can cause the Big Kernel Lock (BKL) to be taken, which
	 * can have significant performance penalties system-wide.  By providing
	 * an unlocked ioctl() method the BKL will not be taken, but the driver
	 * becomes responsible for its own locking.  Furthermore, the lock can be
	 * broken down per A2818 so that multiple threads accessing different CONET
	 * chains do not contend with one another during ioctl() calls.
	 */
	mutex_lock(&s->ioctl_lock);
	ret = (long) ioctl_v1718(inode, file, cmd, arg);
	mutex_unlock(&s->ioctl_lock);

	return ret;
}
#endif

static ssize_t
write_v1718(struct file *file, const char *buffer,
            size_t count, loff_t * ppos)
{
	struct v1718_usb_data *v1718;

	unsigned long copy_size;
	unsigned long bytes_written = 0;
	unsigned int partial;

	int result = 0;
	int maxretry;
	int errn = 0;

	v1718 = (struct v1718_usb_data *)file->private_data;

	down(&(v1718->lock));
        /* Sanity check to make sure v1718 is connected, powered, etc */
        if ( v1718 == NULL ||
             v1718->present == 0 ||
             v1718->v1718_dev == NULL )
	{
		up(&(v1718->lock));
		return -ENODEV;
	}

	do {
		unsigned long thistime;
		char *obuf = v1718->obuf;

		thistime = copy_size =
		    (count >= OBUF_SIZE) ? OBUF_SIZE : count;
		if (copy_from_user(v1718->obuf, buffer, copy_size)) {
			errn = -EFAULT;
			goto error;
		}
//		maxretry = 5;
		maxretry = 1;   // TEMP - 1 volta e' sufficiente ?
		while (thistime) {
			if (!v1718->v1718_dev) {
				errn = -ENODEV;
				goto error;
			}
			if (signal_pending(current)) {
				up(&(v1718->lock));
				return bytes_written ? bytes_written : -EINTR;
			}

			result = usb_bulk_msg(v1718->v1718_dev,
					 usb_sndbulkpipe(v1718->v1718_dev, 2),
//					 obuf, thistime, &partial, 5 * HZ);
					 obuf, thistime, &partial, 1 * HZ);     // TEMP - 1 sec basta?

			dbg("write stats: result:%d thistime:%lu partial:%u",
			     result, thistime, partial);

			if (result == USB_ST_TIMEOUT) {	/* NAK - so hold for a while */
				if (!maxretry--) {
					errn = -ETIME;
					goto error;
				}
				interruptible_sleep_on_timeout(&v1718-> wait_q, NAK_TIMEOUT);
				continue;
//Rev 0.2				
//			} else if (!result & partial) {
			} else if (!result && partial) {
				obuf += partial;
				thistime -= partial;
			} else
				break;
		};
		if (result) {
			err("Write Whoops - %x", result);
			errn = -EIO;
			goto error;
		}
		bytes_written += copy_size;
		count -= copy_size;
		buffer += copy_size;
	} while (count > 0);

	up(&(v1718->lock));

	return bytes_written ? bytes_written : -EIO;

error:
	up(&(v1718->lock));
	return errn;
}

static ssize_t
read_v1718(struct file *file, char *buffer, size_t count, loff_t * ppos)
{
	struct v1718_usb_data *v1718;
	ssize_t read_count;
	unsigned int partial;
	int this_read;
	int result;
//	int maxretry = 10;              // TEMP - Prova per abortire prima 
	int maxretry = 1;	
        char *ibuf;

	v1718 = (struct v1718_usb_data *)file->private_data;

	down(&(v1718->lock));
	/* Sanity check to make sure v1718 is connected, powered, etc */
        if ( v1718 == NULL ||
             v1718->present == 0 ||
             v1718->v1718_dev == NULL )
	{
		up(&(v1718->lock));
		return -ENODEV;
	}

	ibuf = v1718->ibuf;

	read_count = 0;

	while (count > 0) {
		if (signal_pending(current)) {
			up(&(v1718->lock));
			return read_count ? read_count : -EINTR;
		}
		if (!v1718->v1718_dev) {
			up(&(v1718->lock));
			return -ENODEV;
		}
		this_read = (count >= IBUF_SIZE) ? IBUF_SIZE : count;

		result = usb_bulk_msg(v1718->v1718_dev,
				      usb_rcvbulkpipe(v1718->v1718_dev, 6),
				      ibuf, this_read, &partial,
//				      (int) (HZ * 8));
				      (int) (HZ * 1)); // TEMP - 1 secondo credo che basti 

		dbg("read stats: result:%d this_read:%u partial:%u",
		       result, this_read, partial);

		if (partial) {
			count = this_read = partial;
		} else if (result == USB_ST_TIMEOUT || result == 15) {	/* FIXME: 15 ??? */
			if (!maxretry--) {
				up(&(v1718->lock));
				err("read_v1718: maxretry timeout");
				return -ETIME;
			}
			interruptible_sleep_on_timeout(&v1718->wait_q,
						       NAK_TIMEOUT);
			continue;
		} else if (result != USB_ST_DATAUNDERRUN) {
			up(&(v1718->lock));
			err("Read Whoops - result:%u partial:%u this_read:%u",
			     result, partial, this_read);
			return -EIO;
		} else {
			up(&(v1718->lock));
			return (0);
		}

		if (this_read) {
			if (copy_to_user(buffer, ibuf, this_read)) {
				up(&(v1718->lock));
				return -EFAULT;
			}
			count -= this_read;
			read_count += this_read;
			buffer += this_read;
		}
	}
	up(&(v1718->lock));
	return read_count;
}

static struct
file_operations usb_v1718_fops = {
	owner:		THIS_MODULE,            
	read:		read_v1718,
	write:		write_v1718,
	ioctl:		ioctl_v1718,
// Rev. 0.4
#if LINUX_VERSION_CODE >= VERSION(2,6,11)
        unlocked_ioctl: unlocked_ioctl_v1718,
#endif
	open:		open_v1718,
	release:	close_v1718,
};

#if LINUX_VERSION_CODE >= VERSION(2,5,0)

static struct usb_class_driver usb_v1718_class = {
  .name = "usb/v1718_%d",
  .fops = &usb_v1718_fops ,
#if LINUX_VERSION_CODE <= VERSION(2,6,13)
  .mode = S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP ,
#endif
  .minor_base = V1718_MINOR_BASE ,
};

	
static int probe_v1718(struct usb_interface *intf,
                         const struct usb_device_id *id)
#else
static void *probe_v1718(struct usb_device *dev, unsigned int ifnum,
                         const struct usb_device_id *id)
#endif						 
{

#if LINUX_VERSION_CODE >= VERSION(2,5,0)
	struct usb_device *dev = interface_to_usbdev(intf);
	int retval= 0;
#endif
	struct v1718_usb_data *v1718 = NULL;
// Rev 0.2	
//	char name[12];
	int minor;

	info("CAEN V1718 found at address %d", dev->devnum);

	down(&minor_table_mutex);
	for( minor = 0; minor < MAX_DEVICES; ++minor ) {
		if( minor_table[minor] == NULL )
			break;
	}
	if( minor >= MAX_DEVICES ) {
		info("Too many devices");
		goto exit;
	}

	v1718 = kmalloc(sizeof(struct v1718_usb_data), GFP_KERNEL);
	if (v1718 == NULL) {
		err("Out of memory");
		goto exit;
	}
	memset(v1718, 0x00, sizeof(*v1718));
	minor_table[minor] = v1718;
#if LINUX_VERSION_CODE >= VERSION(2,5,0)
	if( usb_register_dev(intf, &usb_v1718_class))
  	{
    	err("probe_v1718: Not able to get a minor for this device.");
		goto error;
	}
#endif	
	v1718->present = 1;
	v1718->v1718_dev = dev;
	v1718->minor = minor;

	if (!(v1718->obuf = (char *) kmalloc(OBUF_SIZE, GFP_KERNEL))) {
		err("probe_v1718: Not enough memory for the output buffer");
		goto error;
	}
	dbg("probe_v1718: obuf address:%p", v1718->obuf);

	if (!(v1718->ibuf = (char *) kmalloc(IBUF_SIZE, GFP_KERNEL))) {
		err("probe_v1718: Not enough memory for the input buffer");
		goto error;
	}
	dbg("probe_v1718: ibuf address:%p", v1718->ibuf);

#if LINUX_VERSION_CODE < VERSION(2,5,0)
	v1718->devfs = devfs_register(usb_devfs_handle, "v1718_0",
				      DEVFS_FL_DEFAULT, USB_MAJOR,
				      V1718_MINOR_BASE + v1718->minor,
				      S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP |
				      S_IWGRP, &usb_v1718_fops, NULL);
	if (v1718->devfs == NULL)
		dbg("probe_v1718: device node registration failed");

	init_MUTEX(&(v1718->lock));
#else

	init_MUTEX(&(v1718->lock));
	usb_set_intfdata (intf, v1718);
	
#endif	
	goto exit;

error:
#if LINUX_VERSION_CODE >= VERSION(2,5,0)
	usb_deregister_dev(intf, &usb_v1718_class);
	retval= -ENOMEM;

#endif
	v1718_delete(v1718);
	v1718 = NULL;

exit:
	up(&minor_table_mutex);
#if LINUX_VERSION_CODE >= VERSION(2,5,0)
	return retval;
#else
	return v1718;
#endif	
}

#if LINUX_VERSION_CODE >= VERSION(2,5,0)
static void disconnect_v1718(struct usb_interface *intf)
#else
static void disconnect_v1718(struct usb_device *dev, void *ptr)
#endif
{
#if LINUX_VERSION_CODE >= VERSION(2,5,0)
	struct v1718_usb_data *v1718 = usb_get_intfdata (intf);
#else
	struct v1718_usb_data *v1718 = (struct v1718_usb_data *) ptr;
#endif
	int minor;

	down(&minor_table_mutex);
	down(&(v1718->lock));

	minor = v1718->minor;

#if LINUX_VERSION_CODE >= VERSION(2,5,0)
	usb_set_intfdata (intf, NULL);
	if (v1718) {
		usb_deregister_dev(intf, &usb_v1718_class);
//rev 0.4: BugFix
//  		down(&(v1718->lock));
		if (v1718->open_count != 0) {
			/* better let it finish - the release will do whats needed */
			v1718->v1718_dev = NULL;
			up(&(v1718->lock));
		} else {
			up(&(v1718->lock));
			v1718_delete(v1718);
		}
	}
#else
	devfs_unregister(v1718->devfs);

	if (v1718->open_count != 0) {
		/* better let it finish - the release will do whats needed */
		v1718->v1718_dev = NULL;
		up(&(v1718->lock));
	} else {
		up(&(v1718->lock));
		v1718_delete(v1718);
	}
#endif

	info("CAEN #%d V1718 disconnected.", minor);

	up(&minor_table_mutex);
}

static struct usb_device_id v1718_table [] = {
	{ USB_DEVICE(0x0547, 0x1002) }, 		/* CAEN V1718 */
	{ }						/* Terminating entry */
};

MODULE_DEVICE_TABLE (usb, v1718_table);

// Rev 0.4
static struct usb_driver v1718_driver = {
#if LINUX_VERSION_CODE >= VERSION(2,5,0) & LINUX_VERSION_CODE < VERSION(2,6,15)
	owner:		THIS_MODULE,
#endif	
	name:		"v1718",
	probe:		probe_v1718,
	disconnect:	disconnect_v1718,
#if LINUX_VERSION_CODE < VERSION(2,5,0)
	fops:		&usb_v1718_fops,
	minor:		V1718_MINOR_BASE,
#endif	
	id_table:	v1718_table,
};

#if LINUX_VERSION_CODE >= VERSION(2,5,0)
static int __init usb_v1718_init(void)
#else
static int usb_v1718_init(void)
#endif
{
	if (usb_register(&v1718_driver) < 0)
		return -1;
	info(DRIVER_VERSION ":" DRIVER_DESC);

	return 0;
}


#if LINUX_VERSION_CODE >= VERSION(2,5,0)
static void __exit usb_v1718_cleanup(void)
#else
static void usb_v1718_cleanup(void)
#endif
{
//Dbg
//printk( "usb_v1718_cleanup enter ...\n");
	usb_deregister(&v1718_driver);
//Dbg
//printk( "usb_v1718_cleanup leave ...\n");
}

module_init(usb_v1718_init);
module_exit(usb_v1718_cleanup);

MODULE_AUTHOR( DRIVER_AUTHOR );
MODULE_DESCRIPTION( DRIVER_DESC );
MODULE_LICENSE("GPL");


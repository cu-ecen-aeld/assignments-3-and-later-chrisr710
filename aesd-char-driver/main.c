/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
#include "aesd-circular-buffer.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Your Name Here"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
	//Okay we are given the inode, and we are given the filepointer.
	//that file pointer is that big structure.
	//we need to fill in the big pointer with our aesd_dev struct.
	
	
	//make an internal dev struct of the type aesd_dev. This is JUST so we can use it to get how it maps in memory. For container_of.
	struct aesd_dev *dev; /* device information */
	
	dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    //(1)ptr – the pointer to the member, which is the cdev strcuty struct.(2) the type. (3) the name of the member)
	//So now DEV = 
	filp->private_data = dev;
	//here is what happened. aesd_device was a struct which had a cdev in it.
	//when we registered the device, we passed a pointer to a member of that global struct, and this allows us to 
	//access other stuff related to it.
	//Seems like we could have initialized that other stuff as global vars too, rather than this complex method.
	//however, 

	
	/**
     * TODO: handle open
     */
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
	return(0);
    /**
     * TODO: handle release
     */
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
	ssize_t retval = 0;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
	/**
     * TODO: handle read
     */
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
	//this function handles WRITES to the dev (reads from my perspective)
    ssize_t retval = -ENOMEM;
    PDEBUG("writey %zu bytes with offset %lld",count,*f_pos);
	char * mybuffer = kmalloc(100 * sizeof(char), GFP_KERNEL);
	PDEBUG("kmalloc done");
	struct aesd_dev *dev = filp->private_data;
    retval = 0;
    copy_to_user(mybuffer,buf,count);
	PDEBUG("COPIED TO MY BUF\n");
	for (int i=0; i<count;i++){PDEBUG("CHAR");PDEBUG("i=%d",i);}
	PDEBUG("fpos SET");
	*f_pos = 4; 
	retval=count;//NEED TO DO THIS OTHERWISE ENDLESS LOOP!!!
    /**
     * TODO: handle write
     */
    return retval;
}
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
	//#1
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    /**
     * TODO: initialize the AESD specific portion of the device
	 This means the locks. Maybe it means the buffer too, but I think that will come in on the read part.
     */

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);

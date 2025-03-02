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
	/*
	You should use the position specified in the read to determine the location and number of bytes to return.

    You should honor the count argument by sending only up to the first “count” bytes back of the available bytes remaining.
	*/
	ssize_t retval = 0;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
	//THE BUFFER: dev->circ_buf
	struct aesd_dev *dev=filp->private_data; //to get the ptrs
	//FIND THE ENTRY TO START READING FROM
	//need logic here to get to the NEXT unconsumed entry, which will be the entry that if AFTER fpos
	loff_t entry_offset_byte_rtn;
	struct aesd_buffer_entry *temp_entry;
	if (*f_pos == 0){
		PDEBUG("FPOS IS ZERO, MUST BE FIRST RUN");
		temp_entry=aesd_circular_buffer_find_entry_offset_for_fpos(dev->circ_buf,1,(size_t *)&entry_offset_byte_rtn);
	}
	else{
		PDEBUG("FPOS IS NOT ZERO");
		temp_entry=aesd_circular_buffer_find_entry_offset_for_fpos(dev->circ_buf,*f_pos + 1,(size_t *)&entry_offset_byte_rtn);
	}
	size_t size_of_current_buffer=temp_entry->size;
	if (size_of_current_buffer + *f_pos >= count){ //current entry has everything we need!
		PDEBUG("CURRENT BUFFER HAS EVERYTHIN WE NEED");
		aesd_circular_buffer_find_entry_offset_for_fpos(dev->circ_buf,count,(size_t *)&entry_offset_byte_rtn);
		copy_to_user(buf,temp_entry->buffptr,entry_offset_byte_rtn);
		*f_pos = *f_pos + entry_offset_byte_rtn;
		PDEBUG("Set offset to %ld",*f_pos);
		PDEBUG("returning %ld",entry_offset_byte_rtn);
		return(entry_offset_byte_rtn);
	}
	else{//current entry is not enough to get us over the edge
		copy_to_user(buf,temp_entry->buffptr,size_of_current_buffer);
		*f_pos = *f_pos + size_of_current_buffer;
		PDEBUG("Set offset to %ld",*f_pos);
		return(size_of_current_buffer);
	}

}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
	struct aesd_buffer_entry dummy; //just so I can get the size of one of these. Probably a better way.
	struct aesd_dev *dev=filp->private_data; //to get the ptrs
	struct aesd_buffer_entry *buf_entry;
	
	if (dev->read_buf == NULL)
		{dev->read_buf = kmalloc(sizeof(dummy),GFP_KERNEL);
		 
	}
	
	buf_entry=dev->read_buf;
	if (dev->read_buf->buffptr == NULL)
		{//PDEBUG("bufprt was null");
		dev->read_buf->buffptr = kmalloc(count,GFP_KERNEL);
		 dev->read_buf->size=0;
	}
	else {//PDEBUG("buffptr was not null");
		//PDEBUG("SIZE IS: %ld",dev->read_buf->size);
		//PDEBUG("increasing buff size by %ld",count);
		char *new_ptr=NULL;
		new_ptr=krealloc(dev->read_buf->buffptr,dev->read_buf->size+count,GFP_KERNEL);
		dev->read_buf->buffptr=new_ptr;
	}
    
	ssize_t retval = -ENOMEM;
    
	//PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
	//PDEBUG("address of dev = %p",dev);
	//PDEBUG("ppaddress of dev->read_buf=%p",buf_entry);
	PDEBUG("address of a new circular buffer entry=%p",buf_entry);
	PDEBUG("address of buffer within it=%p",buf_entry->buffptr);
	
    retval = 0;
    
	retval=copy_from_user(dev->read_buf->buffptr + dev->read_buf->size,buf,count); //WHY IS THIS BACKWARDS AND WORKS?
	//PDEBUG("COPIED TO MY BUFER, returned %ld\n",retval);
	bool terminated=false;
	char * currdata=dev->read_buf->buffptr;
	currdata+=dev->read_buf->size;
	for (int i=0; i<count;i++){
		char mychar=currdata[i];
		if (mychar=='\n'){
			terminated=true;
		}
		
		//PDEBUG("CHAR[%d]:%c",i,mychar);
		//PDEBUG("i=%d",i);
		dev->read_buf->size++;
		}
	if (terminated)
		{PDEBUG("terminated");
		for (int i=0;i<dev->read_buf->size;i++)
			{//PDEBUG("%c",dev->read_buf->buffptr[i]);
			}
		PDEBUG("adding a new entry at %p",buf_entry);
		char * to_free=NULL;
		to_free = aesd_circular_buffer_add_entry(dev->circ_buf,buf_entry);
		if (to_free){
			PDEBUG("Freeing old buffer entry at %p",to_free);
			kfree(to_free);
		}
		//kfree(dev->read_buf->buffptr);
		dev->read_buf=NULL;
		
		}
	else
		{PDEBUG("NOT TERMINATED");
		}
	
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
	struct aesd_circular_buffer dummy;
	dev->circ_buf=kmalloc(sizeof(dummy),GFP_KERNEL);
	dev->circ_buf->full=false;
	PDEBUG("circular buffer init at %p",dev->circ_buf);
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

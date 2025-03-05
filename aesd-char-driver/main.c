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
#include <linux/mutex.h>
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


//apparently these functions just set fpos
loff_t aesd_llseek(struct file *filp, loff_t off, int whence)
{
	struct aesd_dev *dev = filp->private_data;
	size_t newpos;
	PDEBUG("WHENCE IS %d",whence);
	switch(whence) {
	  case 0: /* SEEK_SET */
	    PDEBUG("POSITION SEEK SET");
		newpos = off;
		break;

	  case 1: /* SEEK_CUR */
		newpos = filp->f_pos + off;
		 PDEBUG("POSITION SEEK CUR");
		break;

	  case 2: /* SEEK_END */
		newpos = get_length_of_all_entries_in_buffer(dev->circ_buf) + off;
		 PDEBUG("POSITION SEEK END");
		break;

	  default: /* can't happen */
	    PDEBUG("CANT HAPPEN. YET IT DID");
		return -EINVAL;
	}
	if (newpos < 0) return -EINVAL;
	PDEBUG("SEEK RETURNING FPOS %ld",newpos);
	filp->f_pos = newpos;
	return newpos;
}



ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{	

	size_t te=0;
	uint8_t ix;
	struct aesd_buffer_entry *tentry;
	
	ssize_t retval = 0;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
	//THE BUFFER: dev->circ_buf
	struct aesd_dev *dev=filp->private_data; //to get the ptrs
	/*AESD_CIRCULAR_BUFFER_FOREACH(tentry,dev->circ_buf,ix) {
	   PDEBUG("INDEX= %ld, CHAR0=%c", get_index_of_current_entry(tentry,dev->circ_buf),(char)*(tentry->buffptr));
	}
	*/
	ssize_t total_size_of_buffers=get_length_of_all_entries_in_buffer(dev->circ_buf);
	if (count > total_size_of_buffers){  //we must tell YOU how many bytes to request.
		count = total_size_of_buffers - *f_pos;
		PDEBUG("reset count to %ld",count);
		//PDEBUG("count is for greater than size of all buffers, must be using cat, etc.");
		}
	
	if (count == 0){
		PDEBUG("You have consumed all in the buffers");
		return(0); //you have gotten all we have
	}
	
	
	//FIND THE ENTRY TO START READING FROM
	//need logic here to get to the NEXT unconsumed entry, which will be the entry that if AFTER fpos
	loff_t entry_offset_byte_rtn;
	struct aesd_buffer_entry *temp_entry;
	PDEBUG("searching for entry at offset %ld",(*f_pos));
	//darnit, this won't work if searching for one byte. OR will it?
	temp_entry=aesd_circular_buffer_find_entry_offset_for_fpos(dev->circ_buf,(*f_pos),(size_t *)&entry_offset_byte_rtn);
	
	
	if (temp_entry == 0){PDEBUG("RETURNING 0 BECAUSE TEMP_ENTRY WAS NULL");
		return(0);}
		
	PDEBUG("We chose entry with index %ld because it was what was returned for offset %ld",get_index_of_current_entry(temp_entry, dev->circ_buf),(*f_pos));
	size_t size_of_current_buffer=temp_entry->size;
	PDEBUG("check if size of current entry[%ld]: %ld is >= the count %ld",get_index_of_current_entry(temp_entry, dev->circ_buf), size_of_current_buffer,count);
	PDEBUG("THE PLACE WE WILL START COPYING ON THIS IS %ld",entry_offset_byte_rtn);
	PDEBUG("NOW RETURN THE BYTES FROM THIS ENTRY, UP TO THE CURRENT LIMIT, WHICH IS THE SMALLER OF SIZE OF THE CURRENT BUFFER, OR COUNT!");
	size_t bytes_to_copy=count;
	if ((size_of_current_buffer - entry_offset_byte_rtn) < count){ //we are going to copy more stuff later,
		bytes_to_copy = size_of_current_buffer - entry_offset_byte_rtn;} //just copy everything out of the buffer except what was before the byte return.
		
	if ((size_of_current_buffer - entry_offset_byte_rtn) > count){ //we have more than we need in this buffer
    bytes_to_copy = count; //Only copy out as much as we need		
	}
	PDEBUG("WE HAVE DETERMINED THAT WE SHOULD COPY %ld bytes from this starting at offset %ld", bytes_to_copy, entry_offset_byte_rtn);
	
	//copy the calculated number of bytes to the user
	size_t returner = copy_to_user(buf,temp_entry->buffptr + entry_offset_byte_rtn, bytes_to_copy);
	PDEBUG("COPYING RETURNED %ld",returner);
	*f_pos = *f_pos + bytes_to_copy;
	PDEBUG("FPOS set to %ld, returning %ld",*f_pos,bytes_to_copy);
	return(bytes_to_copy);
	
/*
	
	if (size_of_current_buffer >= (count)){ //current entry has everything we need!
		PDEBUG("CURRENT BUFFER HAS EVERYTHIN WE NEED, will search for entry with offset %ld",(*f_pos) + 1 );
		
		temp_entry = aesd_circular_buffer_find_entry_offset_for_fpos(dev->circ_buf, (*f_pos) + 1,(size_t *)&entry_offset_byte_rtn);
		size_t returner = copy_to_user(buf + entry_offset_byte_rtn - 1,temp_entry->buffptr,count);
		if (returner != 0) {return -EFAULT;}
		PDEBUG("offset within this buffer: %ld",entry_offset_byte_rtn);
		*f_pos = *f_pos + count;
		PDEBUG("Set fpos to %ld",*f_pos);
		PDEBUG("returning this many bytes: %ld",count);
		return(count);
	}
	else{//current entry is not enough to get us over the edge
			size_t returner = copy_to_user(buf + entry_offset_byte_rtn -1,temp_entry->buffptr,size_of_current_buffer);
			if (returner != 0) {return -EFAULT;}
		PDEBUG("offset within this buffer: %ld",entry_offset_byte_rtn);
		*f_pos = *f_pos + size_of_current_buffer;
		PDEBUG("Set offset to %ld",*f_pos);
		PDEBUG("returning %ld",size_of_current_buffer);
		return(size_of_current_buffer);
	}
*/
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
	PDEBUG("ENTERING WRITE");
	struct aesd_buffer_entry dummy; //just so I can get the size of one of these. Probably a better way.
	struct aesd_dev *dev=filp->private_data; //to get the ptrs
	
	if (mutex_lock_interruptible(&dev->lock))
		{return -ERESTARTSYS;}
	
	
	
	if (dev->read_buf == 0)
		{PDEBUG("MALLOCING 1");
		dev->read_buf = (struct aesd_buffer_entry *)kmalloc(sizeof(dummy),GFP_KERNEL);
		dev->read_buf->buffptr=0;
		dev->read_buf->size=0;
		}
	
	
	if (dev->read_buf->buffptr == 0)
		{PDEBUG("bufprt was null");
	     PDEBUG("MALLOCING 2, size malloced is %ld",count);
		dev->read_buf->buffptr = kmalloc(count,GFP_KERNEL);
		 dev->read_buf->size=0;
	}
	else {PDEBUG("buffptr was not null");
		PDEBUG("SIZE IS: %ld",dev->read_buf->size);
		PDEBUG("increasing buff size by %ld",count);
		PDEBUG("MALLOCING 3, a size of %ld",dev->read_buf->size + count);
		dev->read_buf->buffptr=krealloc(dev->read_buf->buffptr,dev->read_buf->size + count,GFP_KERNEL);	
	}
	ssize_t retval = -ENOMEM;
	PDEBUG("address of a new circular buffer entry=%p",dev->read_buf);
	PDEBUG("address of buffer within it=%p",dev->read_buf->buffptr);
	
    retval = 0;
    
	retval=copy_from_user(dev->read_buf->buffptr + dev->read_buf->size,buf,count); //WHY IS THIS BACKWARDS AND WORKS?
	PDEBUG("COPIED TO MY BUFER, returned %ld\n",retval);
	if (retval != 0) {return(-EFAULT);}
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
		PDEBUG("adding a new entry at %p",dev->read_buf);
		char * to_free=0;
		to_free = aesd_circular_buffer_add_entry(dev->circ_buf,dev->read_buf);
		if (to_free){
			PDEBUG("Freeing old buffer entry at %p",to_free);
			kfree(to_free);
		}
		//kfree(dev->read_buf->buffptr);
		dev->read_buf=0;
		
		}
	else
		{PDEBUG("NOT TERMINATED");
		}
	
	retval=count;//NEED TO DO THIS OTHERWISE ENDLESS LOOP!!!
    
	mutex_unlock(&dev->lock);
    return retval;
}
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
	.llseek =   aesd_llseek,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
	struct aesd_circular_buffer dummy;
	struct aesd_buffer_entry dummy2;
	dev->circ_buf=(struct aesd_circular_buffer*)kmalloc(sizeof(dummy),GFP_KERNEL);
	aesd_circular_buffer_init(dev->circ_buf);
	dev->circ_buf->full=false;
    dev->read_buf=(struct aesd_buffer_entry*)kmalloc(sizeof(dummy2),GFP_KERNEL);
    dev->read_buf->size=0;
	dev->read_buf->buffptr=0;
	if (dev->read_buf->buffptr==0)
		{PDEBUG("THE BUFFER IS NULL AND THE SIZE IS %ld",dev->read_buf->size);}
	else
		{PDEBUG("THE BUFFER IS NOT NULL AND THE SIZE IS %ld",dev->read_buf->size);}
	PDEBUG("circular buffer init at %p",dev->circ_buf);
	mutex_init(&dev->lock);
	PDEBUG("initialized semaphore! ");
	PDEBUG("Set up the read buffer");
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
	
	
	//aesd_device.read_buf->size=0;
    

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
	uint8_t index;
	struct aesd_buffer_entry *entry;
	AESD_CIRCULAR_BUFFER_FOREACH(entry,aesd_device.circ_buf,index) {
	   PDEBUG("Freed entry at %p",entry->buffptr);
       kfree(entry->buffptr);
	}
	//free read buffer
	//PDEBUG("Free read buffer at %p",&aesd_device.read_buf);
	//kfree(&(aesd_device.read_buf));
	//free lock?

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */

    unregister_chrdev_region(devno, 1);
}








module_init(aesd_init_module);
module_exit(aesd_cleanup_module);

/*
 * aesd_ioctl.h
 *
 *  Created on: Oct 23, 2019
 *      Author: Dan Walkes
 *
 *  @brief Definitins for the ioctl used on aesd char devices for assignment 9
 */

#ifndef AESD_IOCTL_H
#define AESD_IOCTL_H

#ifdef __KERNEL__
#include <asm-generic/ioctl.h>
#include <linux/types.h>
#else
#include <sys/ioctl.h>
#include <stdint.h>
#include <linux/module.h>





#endif
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
#include "aesd-circular-buffer.h"
#include <linux/mutex.h>
#include "aesd_ioctl.h"

/**
 * A structure to be passed by IOCTL from user space to kernel space, describing the type
 * of seek performed on the aesdchar driver
 */
struct aesd_seekto {
    /**
     * The zero referenced write command to seek into
     */
    uint32_t write_cmd;
    /**
     * The zero referenced offset within the write
     */
    uint32_t write_cmd_offset;
	
	
	size_t length_copied;
	
	char * special_buffer;
};
long int aesd_ioctl(struct file *file, unsigned int cmd, long unsigned int arg);
// Pick an arbitrary unused value from https://github.com/torvalds/linux/blob/master/Documentation/userspace-api/ioctl/ioctl-number.rst
#define AESD_IOC_MAGIC 0x16

// Define a write command from the user point of view, use command number 1
#define AESDCHAR_IOCSEEKTO _IOWR(AESD_IOC_MAGIC, 1, struct aesd_seekto)
/**
 * The maximum number of commands supported, used for bounds checking
 */
#define AESDCHAR_IOC_MAXNR 1

#endif /* AESD_IOCTL_H */

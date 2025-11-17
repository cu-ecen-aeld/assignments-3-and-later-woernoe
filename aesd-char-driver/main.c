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
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("woernoe"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    /**
     * TODO: handle open
     */
    struct aesd_dev *dev;
    dev = conainter_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev;


    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
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
    struct aesd_dev *dev = filp->private_data;
    struct aesd_circular_buffer *cb = dev->data;

    size_t b_fpos = *f_pos;
    size_t b_start = 0;
    size_t totcpy = 0;

    struct aesd_buffer_entry *pEntry = NULL;

    if (mutex_lock_interruptible(&dev->lock))
         return -ERESTARTSYS;

    // walk buffer
    while ( ( pEntry = aesd_circular_buffer_find_entry_offset_for_fpos( cb, b_fpos, &b_start)) != NULL ) 
    {
        // Nr of bytes to copy
        size_t cpy_bytes =  be->size - b_start;
                          
        // any limitations on amount of receive buffer
        if ( (totcpy + cpy_bytes) > count ) 
        {
            // only the remaining nrs
            cpy_bytes = count - totcpy;
        }
        
        // copy to userspace 
        if (copy_to_user(buf + totcpy, pEntry->buffptr + b_start, cpy_bytes)) {
	    retval = -EFAULT;
	    goto out;
	}         

        // pointer to next bufferentry
        b_fpos += cpy_bytes;          
         
        // amount of bytes copied 
        totcpy += cpy_bytes;
        // exceeds the limit of userbuffer
        if ( totcpy >= count) {
             break;
        }
    }
    
    retval = totcpy;  

out:
    mutex_unlock(&dev->lock);


    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write
     */
    ssize_t retval = -ENOMEM;

    struct aesd_dev *dev = filp->private_data;
    struct aesd_circular_buffer *cb = dev->data;
    char *kmem = NULL;

    if (mutex_lock_interruptible(&dev->lock))
		return -ERESTARTSYS;

    if (count > 0 )
    {
        
        ssize_t rcount = count;      // # Bytes to request

        if (dev->tmpDataLen != 0)
        {
            rcount += dev->tmpDataLen;

            char *kmem = kmalloc(rcount + 1, GFP_KERNEL);
            if (kmem == NULL)
                goto out_nomem2;

            memcpy(kmem, dev->tmpData, dev->tmpData.tmpDataLen);
 
            copy_from_user(kmem + dev->tmpDataLen, buf, count);

            kfree(dev->tmpData);
             
            dev->tmpData = kmem;
            dev->tmpDataLen = rcount;
        }
        else {
            char *kmem = kmalloc(count + 1, GFP_KERNEL);
            if (kmem == NULL)
                goto out_nomem;

            copy_from_user(kmem, buf, count);

            dev->tmpData = kmem;
            dev->tmpDataLen = count;
            
        } 

        if (*(dev->tmpData + count -1) == '\n') 
        {

             // temp block on stack           
             struct aesd_buffer_entry nEntry;
             
             nEntry.buffptr = dev->tmpData;
             nEntry->size = dev->tmpDataCount;
             
             dev->tmpData = NULL;
             dev->tmpDataCount = 0;

             aesd_circular_buffer_add_entry( cb, nentry); 
             
        }
        
        retval = count;
    }
    else {
       // stopped
       if (dev->tmpData != NULL )
           goto oetnomem2;
    
       }
    }
    goto out; 
 

outnomem2:
    kfree(dev->tmpData);
    dev->tmpData = NULL;
    dev->tmpDataCount = 0;

out:
    mutex_unlock(&dev->lock);

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
     */
    aesd_circular_buffer_init(&(aesd_device->data));       // we
    aesd_device.tmpData = NULL;                            // we
    aesd_device.tmpDataLen = 0;

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
    if (mutex_lock_interruptable(&aesd_device->lock)        
	return -ERESTARTSYS;
    aesd_circular_buffer_free_all(&(aesd_device->data));
    if (aesd_device.tmpData != NULL) {
        kfree(aesd_device.tmpData);
        aesd_device.tmpData = NULL;
    }
    mutex_unlock(&aesd_device->lock);

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);

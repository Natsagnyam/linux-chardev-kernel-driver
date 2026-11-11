#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/types.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Natsagnyam Namkhai");
MODULE_DESCRIPTION("Out-of-tree persistent character device driver");
MODULE_VERSION("0.3");

#define DEVICE_NAME "demodev"
#define BUF_LEN 1024

static int major_number;
static struct class *demodev_class = NULL;
static struct device *demodev_device = NULL;

/* Persistent global storage buffer across file openings */
static char *global_kernel_buf = NULL;
static size_t global_datalen = 0;

/* File operations: open() */
static int demodev_open(struct inode *inodep, struct file *filep)
{
    pr_info("demodev: device opened successfully\n");
    return 0;
}

/* File operations: release() */
static int demodev_release(struct inode *inodep, struct file *filep)
{
    pr_info("demodev: device closed successfully\n");
    return 0;
}

/* File operations: read() */
static ssize_t demodev_read(struct file *filep, char __user *buf, size_t count, loff_t *ppos)
{
    size_t datalen;
    size_t not_copied;

    if (*ppos >= global_datalen)
        return 0; // End of file

    datalen = global_datalen - *ppos;
    if (count < datalen)
        datalen = count;

    not_copied = copy_to_user(buf, global_kernel_buf + *ppos, datalen);
    if (not_copied)
        return -EFAULT;

    *ppos += datalen;
    return datalen;
}

/* File operations: write() */
static ssize_t demodev_write(struct file *filep, const char __user *buf, size_t count, loff_t *ppos)
{
    size_t not_copied;

    if (count > BUF_LEN)
        count = BUF_LEN;

    *ppos = 0; // Reset offset on new write

    not_copied = copy_from_user(global_kernel_buf + *ppos, buf, count);
    if (not_copied)
        return -EFAULT;

    global_datalen = count;
    *ppos += count;

    pr_info("demodev: stored %zu bytes in kernel buffer\n", count);
    return count;
}

/* File operations mapping */
static const struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = demodev_open,
    .release = demodev_release,
    .read    = demodev_read,
    .write   = demodev_write,
};

/* Module Initialization */
static int __init demodev_init(void)
{
    pr_info("demodev: initializing persistent character device driver\n");

    /* Allocate global persistent buffer */
    global_kernel_buf = kzalloc(BUF_LEN, GFP_KERNEL);
    if (!global_kernel_buf)
        return -ENOMEM;

    /* 1. Dynamically allocate major number */
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        kfree(global_kernel_buf);
        pr_err("demodev: failed to register major number\n");
        return major_number;
    }

    /* 2. Create device class */
    demodev_class = class_create(DEVICE_NAME);
    if (IS_ERR(demodev_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        kfree(global_kernel_buf);
        pr_err("demodev: failed to create device class\n");
        return PTR_ERR(demodev_class);
    }

    /* 3. Automatically create device node /dev/demodev */
    demodev_device = device_create(demodev_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(demodev_device)) {
        class_destroy(demodev_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        kfree(global_kernel_buf);
        pr_err("demodev: failed to create device node\n");
        return PTR_ERR(demodev_device);
    }

    pr_info("demodev: registered successfully with major number %d\n", major_number);
    return 0;
}

/* Module Exit */
static void __exit demodev_exit(void)
{
    device_destroy(demodev_class, MKDEV(major_number, 0));
    class_destroy(demodev_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    kfree(global_kernel_buf);
    pr_info("demodev: module unloaded successfully\n");
}

module_init(demodev_init);
module_exit(demodev_exit);
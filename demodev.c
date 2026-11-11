#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/ioctl.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Natsagnyam Namkhai");
MODULE_DESCRIPTION("Out-of-tree persistent character device driver with mutex synchronization and ioctl");
MODULE_VERSION("0.5");

#define DEVICE_NAME "demodev"
#define BUF_LEN 1024

/* ioctl definitions */
#define DEMODEV_IOC_MAGIC 'k'

struct demodev_stats {
    unsigned long write_count;
    unsigned long read_count;
    size_t current_len;
};

#define DEMODEV_IOC_RESET      _IO(DEMODEV_IOC_MAGIC, 1)
#define DEMODEV_IOC_GET_STATS  _IOR(DEMODEV_IOC_MAGIC, 2, struct demodev_stats)

static int major_number;
static struct class *demodev_class = NULL;
static struct device *demodev_device = NULL;

/* Persistent global storage buffer and mutex concurrency control */
static char *global_kernel_buf = NULL;
static size_t global_datalen = 0;
static struct mutex demodev_mutex;

/* Driver statistics counters */
static unsigned long total_writes = 0;
static unsigned long total_reads = 0;

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

    if (mutex_lock_interruptible(&demodev_mutex))
        return -ERESTARTSYS;

    if (*ppos >= global_datalen) {
        mutex_unlock(&demodev_mutex);
        return 0; // End of file
    }

    datalen = global_datalen - *ppos;
    if (count < datalen)
        datalen = count;

    // Mutex allows safe sleeping during copy_to_user if page faults occur
    not_copied = copy_to_user(buf, global_kernel_buf + *ppos, datalen);
    if (not_copied) {
        mutex_unlock(&demodev_mutex);
        return -EFAULT;
    }

    *ppos += datalen;
    total_reads++;
    
    mutex_unlock(&demodev_mutex);
    return datalen;
}

/* File operations: write() */
static ssize_t demodev_write(struct file *filep, const char __user *buf, size_t count, loff_t *ppos)
{
    size_t not_copied;

    if (count > BUF_LEN)
        count = BUF_LEN;

    if (mutex_lock_interruptible(&demodev_mutex))
        return -ERESTARTSYS;

    *ppos = 0; // Reset offset on new write
    not_copied = copy_from_user(global_kernel_buf, buf, count);
    if (not_copied) {
        mutex_unlock(&demodev_mutex);
        return -EFAULT;
    }

    global_datalen = count;
    *ppos += count;
    total_writes++;

    mutex_unlock(&demodev_mutex);

    pr_info("demodev: stored %zu bytes under mutex protection\n", count);
    return count;
}

/* File operations: unlocked_ioctl() */
static long demodev_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
    if (_IOC_TYPE(cmd) != DEMODEV_IOC_MAGIC)
        return -ENOTTY;

    if (mutex_lock_interruptible(&demodev_mutex))
        return -ERESTARTSYS;

    switch (cmd) {
    case DEMODEV_IOC_RESET:
        global_datalen = 0;
        memset(global_kernel_buf, 0, BUF_LEN);
        pr_info("demodev: buffer reset via ioctl\n");
        break;

    case DEMODEV_IOC_GET_STATS: {
        struct demodev_stats stats;

        stats.write_count = total_writes;
        stats.read_count = total_reads;
        stats.current_len = global_datalen;

        if (copy_to_user((struct demodev_stats __user *)arg, &stats, sizeof(stats))) {
            mutex_unlock(&demodev_mutex);
            return -EFAULT;
        }
        break;
    }

    default:
        mutex_unlock(&demodev_mutex);
        return -ENOTTY;
    }

    mutex_unlock(&demodev_mutex);
    return 0;
}

/* File operations mapping */
static const struct file_operations fops = {
    .owner          = THIS_MODULE,
    .open           = demodev_open,
    .release        = demodev_release,
    .read           = demodev_read,
    .write          = demodev_write,
    .unlocked_ioctl = demodev_ioctl,
};

/* Module Initialization */
static int __init demodev_init(void)
{
    pr_info("demodev: initializing persistent character device driver with mutex\n");

    mutex_init(&demodev_mutex);

    global_kernel_buf = kzalloc(BUF_LEN, GFP_KERNEL);
    if (!global_kernel_buf)
        return -ENOMEM;

    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        kfree(global_kernel_buf);
        pr_err("demodev: failed to register major number\n");
        return major_number;
    }

    demodev_class = class_create(DEVICE_NAME);
    if (IS_ERR(demodev_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        kfree(global_kernel_buf);
        pr_err("demodev: failed to create device class\n");
        return PTR_ERR(demodev_class);
    }

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

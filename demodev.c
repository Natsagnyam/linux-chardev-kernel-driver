#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Canonical Engineering Prep");
MODULE_DESCRIPTION("Out-of-Tree Character Device Driver");
MODULE_VERSION("0.1");

#define DEVICE_NAME "demodev"
#define BUF_LEN 80

static int major;
static struct class *demo_class;
static struct cdev demo_cdev;
static char msg[BUF_LEN];
static short read_pos;

static int dev_open(struct inode *inodep, struct file *filep) {
    read_pos = 0;
    try_module_get(THIS_MODULE);
    return 0;
}

static ssize_t dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset) {
    int bytes_read = 0;
    if (!msg[read_pos]) return 0;
    while (len && msg[read_pos]) {
        put_user(msg[read_pos++], buffer++);
        len--;
        bytes_read++;
    }
    return bytes_read;
}

static ssize_t dev_write(struct file *filep, const char __user *buffer, size_t len, loff_t *offset) {
    int i;
    memset(msg, 0, BUF_LEN);
    if (len > BUF_LEN - 1) len = BUF_LEN - 1;
    for (i = 0; i < len; i++) {
        get_user(msg[i], buffer + i);
    }
    msg[len] = '\0';
    return len;
}

static int dev_release(struct inode *inodep, struct file *filep) {
    module_put(THIS_MODULE);
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
};

static int __init demodev_init(void) {
    dev_t dev_num;
    if (alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME) < 0) return -1;
    major = MAJOR(dev_num);
    cdev_init(&demo_cdev, &fops);
    if (cdev_add(&demo_cdev, dev_num, 1) < 0) {
        unregister_chrdev_region(dev_num, 1);
        return -1;
    }
    demo_class = class_create("demo_class");
    if (IS_ERR(demo_class)) {
        cdev_del(&demo_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(demo_class);
    }
    if (IS_ERR(device_create(demo_class, NULL, dev_num, NULL, DEVICE_NAME))) {
        class_destroy(demo_class);
        cdev_del(&demo_cdev);
        unregister_chrdev_region(dev_num, 1);
        return -1;
    }
    return 0;
}

static void __exit demodev_exit(void) {
    dev_t dev_num = MKDEV(major, 0);
    device_destroy(demo_class, dev_num);
    class_destroy(demo_class);
    cdev_del(&demo_cdev);
    unregister_chrdev_region(dev_num, 1);
}

module_init(demodev_init);
module_exit(demodev_exit);

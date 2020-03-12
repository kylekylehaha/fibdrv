#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include "bigN.h"

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("National Cheng Kung University, Taiwan");
MODULE_DESCRIPTION("Fibonacci engine driver");
MODULE_VERSION("0.1");

#define DEV_FIBONACCI_NAME "fibonacci"

/* MAX_LENGTH is set to 92 because
 * ssize_t can't fit the number > 92
 */
#define MAX_LENGTH 100

// typedef static unsigned long long (*fib_op)(long long k)

static dev_t fib_dev = 0;
static struct cdev *fib_cdev;
static struct class *fib_class;
static DEFINE_MUTEX(fib_mutex);
static ktime_t kt;

static void bigN_assign(bigN *a, bigN *b)
{
    for (int i = 0; i < part_num; i++) {
        a->part[i] = b->part[i];
    }
    return;
}


static void bigN_add(bigN *output, bigN x, bigN y)
{
    memset(output, 0, sizeof(bigN));
    unsigned long long carry = 0;
    for (int i = 0; i < part_num; i++) {
        unsigned long long tmp = carry + x.part[i] + y.part[i];
        output->part[i] = tmp % BASE;
        carry = tmp / BASE;
    }
}

/*s
unsigned int clz(long long k)
{
    unsigned int n = 0;

    if (k == 0)
        return 0;

    if (k <= 0x00000000FFFFFFFF) {
        n += 32;
    }
    if (k <= 0x0000FFFFFFFFFFFF) {
        n += 16;
    }
    if (k <= 0x00FFFFFFFFFFFFFF) {
        n += 8;
    }
    if (k <= 0x0FFFFFFFFFFFFFFF) {
        n += 4;
    }
    if (k <= 0x3FFFFFFFFFFFFFFF) {
        n += 2;
    }
    if (k <= 0x7FFFFFFFFFFFFFFF) {
        n += 1;
    }

    return n;
}

static unsigned long long fib_fast_doubling_clz(long long k)
{
    unsigned int digit = 0;
    int saved = k;
    while (k) {
        digit++;
        k /= 2;
    }
    k = saved;

    unsigned int n = clz(k);
    k << n;

    unsigned long long a, b;
    a = 0;
    b = 1;
    for (unsigned int i = digit; i > 0; i--) {
        unsigned long long t1, t2;
        t1 = a * (2 * b - a);
        t2 = b * b + a * a;
        a = t1;
        b = t2;
        if (k & (1 << (i - 1))) {
            t1 = a + b;
            a = b;
            b = t1;
        }
    }

    return a;
}


static unsigned long long fib_fast_doubling(long long k)
{
    unsigned int digit = 0;
    int saved = k;
    while (k) {
        digit++;
        k /= 2;
    }
    k = saved;
    unsigned long long a, b;
    a = 0;
    b = 1;
    for (unsigned int i = digit; i > 0; i--) {
        unsigned long long t1, t2;
        t1 = a * (2 * b - a);
        t2 = b * b + a * a;
        a = t1;
        b = t2;
        if (k & (1 << (i - 1))) {
            t1 = a + b;
            a = b;
            b = t1;
            // k &= ~(1 << (i - 1));
        }
    }

    return a;
}
*/


static void fib_sequence(long long k, bigN *output)
{
    if (k == 0) {
        memset(output, 0, sizeof(bigN));
        return;
    }
    bigN *f0, *f1, *f2;
    f0 = kmalloc(sizeof(bigN), GFP_KERNEL);
    f1 = kmalloc(sizeof(bigN), GFP_KERNEL);
    memset(f0, 0, sizeof(bigN));
    memset(f1, 0, sizeof(bigN));
    f0->part[0] = 0;
    f1->part[0] = 1;
    for (int i = 2; i <= k; i++) {
        bigN_add(f0, *f0, *f1);
        f2 = f0;
        f0 = f1;
        f1 = f2;
    }

    bigN_assign(output, f1);
    return;
}

/*
static unsigned long long fib_time_proxy(long long k)
{
    kt = ktime_get();
    unsigned long long result = fib_fast_doubling_clz(k);
    kt = ktime_sub(ktime_get(), kt);

    return result;
}
*/

static int fib_open(struct inode *inode, struct file *file)
{
    if (!mutex_trylock(&fib_mutex)) {
        printk(KERN_ALERT "fibdrv is in use");
        return -EBUSY;
    }
    return 0;
}

static int fib_release(struct inode *inode, struct file *file)
{
    mutex_unlock(&fib_mutex);
    return 0;
}

/* calculate the fibonacci number at given offset */
static ssize_t fib_read(struct file *file,
                        char *buf,
                        size_t size,
                        loff_t *offset)
{
    bigN tmp;
    kt = ktime_get();
    fib_sequence(*offset, &tmp);
    kt = ktime_get();
    tmp.kernel_t = ktime_to_ns(kt);
    copy_to_user(buf, &tmp, size);

    return 1;
}

/* write operation is skipped */
static ssize_t fib_write(struct file *file,
                         const char *buf,
                         size_t size,
                         loff_t *offset)
{
    return 1;
}

static loff_t fib_device_lseek(struct file *file, loff_t offset, int orig)
{
    loff_t new_pos = 0;
    switch (orig) {
    case 0: /* SEEK_SET: */
        new_pos = offset;
        break;
    case 1: /* SEEK_CUR: */
        new_pos = file->f_pos + offset;
        break;
    case 2: /* SEEK_END: */
        new_pos = MAX_LENGTH - offset;
        break;
    }

    if (new_pos > MAX_LENGTH)
        new_pos = MAX_LENGTH;  // max case
    if (new_pos < 0)
        new_pos = 0;        // min case
    file->f_pos = new_pos;  // This is what we'll use now
    return new_pos;
}

const struct file_operations fib_fops = {
    .owner = THIS_MODULE,
    .read = fib_read,
    .write = fib_write,
    .open = fib_open,
    .release = fib_release,
    .llseek = fib_device_lseek,
};

static int __init init_fib_dev(void)
{
    int rc = 0;

    mutex_init(&fib_mutex);

    // Let's register the device
    // This will dynamically allocate the major number
    rc = alloc_chrdev_region(&fib_dev, 0, 1, DEV_FIBONACCI_NAME);

    if (rc < 0) {
        printk(KERN_ALERT
               "Failed to register the fibonacci char device. rc = %i",
               rc);
        return rc;
    }

    fib_cdev = cdev_alloc();
    if (fib_cdev == NULL) {
        printk(KERN_ALERT "Failed to alloc cdev");
        rc = -1;
        goto failed_cdev;
    }
    cdev_init(fib_cdev, &fib_fops);
    rc = cdev_add(fib_cdev, fib_dev, 1);

    if (rc < 0) {
        printk(KERN_ALERT "Failed to add cdev");
        rc = -2;
        goto failed_cdev;
    }

    fib_class = class_create(THIS_MODULE, DEV_FIBONACCI_NAME);

    if (!fib_class) {
        printk(KERN_ALERT "Failed to create device class");
        rc = -3;
        goto failed_class_create;
    }

    if (!device_create(fib_class, NULL, fib_dev, NULL, DEV_FIBONACCI_NAME)) {
        printk(KERN_ALERT "Failed to create device");
        rc = -4;
        goto failed_device_create;
    }
    return rc;
failed_device_create:
    class_destroy(fib_class);
failed_class_create:
    cdev_del(fib_cdev);
failed_cdev:
    unregister_chrdev_region(fib_dev, 1);
    return rc;
}

static void __exit exit_fib_dev(void)
{
    mutex_destroy(&fib_mutex);
    device_destroy(fib_class, fib_dev);
    class_destroy(fib_class);
    cdev_del(fib_cdev);
    unregister_chrdev_region(fib_dev, 1);
}

module_init(init_fib_dev);
module_exit(exit_fib_dev);

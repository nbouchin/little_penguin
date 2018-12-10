#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/mutex.h>

MODULE_LICENSE("GPL");

static const char g_s_logname[] = "nbouchin";
static char g_s_chararray[8];
static const ssize_t g_s_logname_size = sizeof(g_s_logname);
static void *ptr;
static size_t ldata;

DEFINE_MUTEX(lock);

ssize_t id_file_read(struct file *filp, char __user *buff, size_t count,
		     loff_t *offp)
{
	if (*offp >= g_s_logname_size)
		return 0;
	if (*offp + count > g_s_logname_size)
		count = g_s_logname_size - *offp;
	if (copy_to_user(buff, g_s_logname + *offp, count) != 0)
		return -EFAULT;
	*offp += count;
	return count;
}

ssize_t id_file_write(struct file *filp, const char __user *buff, size_t count,
		      loff_t *offp)
{
	if (!buff || count != g_s_logname_size) {
		return -EFAULT;
	} else {
		if (copy_from_user(g_s_chararray + *offp, buff, count) != 0) {
			return -EFAULT;
		}
		*offp += count;
	}
	if (strncmp(g_s_logname, g_s_chararray, 8))
		return -EFAULT;
	printk(KERN_INFO "Id debugfs file write is ok.\n");
	return count;
}

ssize_t foo_file_read(struct file *filp, char __user *buff, size_t count,
		      loff_t *offp)
{
	mutex_lock(&lock);
	if (*offp >= PAGE_SIZE) {
		mutex_unlock(&lock);
		return 0;
	}
	if (*offp + count > ldata)
		count = ldata - *offp;
	if (copy_to_user(buff, ptr + *offp, count) != 0) {
		mutex_unlock(&lock);
		return -EFAULT;
	}
	*offp += count;
	mutex_unlock(&lock);
	return count;
}

ssize_t foo_file_write(struct file *filp, const char __user *buff, size_t count,
		       loff_t *offp)
{
	mutex_lock(&lock);
	if (!ptr) {
		if (!(ptr = kmalloc(PAGE_SIZE, GFP_KERNEL))) {
			mutex_unlock(&lock);
			return -EFAULT;
		}
		memset(ptr, 0, PAGE_SIZE);
	}
	if (!buff || !ptr) {
		mutex_unlock(&lock);
		return -EFAULT;
	} else {
		if (copy_from_user(ptr + *offp, buff, count) != 0) {
			mutex_unlock(&lock);
			return -EFAULT;
		}
		*offp += count;
		ldata += count;
	}
	printk(KERN_INFO "Foo debugfs file is ok.\n");
	mutex_unlock(&lock);
	return count;
}

struct dentry *fortytwo = NULL;
struct file_operations id_fops = { .read = id_file_read,
				   .write = id_file_write };
struct file_operations foo_fops = { .read = foo_file_read,
				    .write = foo_file_write };

static int __init misc_init(void)
{
	fortytwo = debugfs_create_dir("fortytwo", NULL);
	debugfs_create_file("id", 0777, fortytwo, NULL, &id_fops);
	debugfs_create_u64("jiffies", 0444, fortytwo, (void *)&jiffies);
	debugfs_create_file("foo", 0644, fortytwo, NULL, &foo_fops);
	printk(KERN_INFO "Debugfs files generated.\n");
	return 0;
}

static void __exit misc_exit(void)
{
	debugfs_remove_recursive(fortytwo);
	printk(KERN_INFO "Cleaning up debugfs files.\n");
}

module_init(misc_init);
module_exit(misc_exit);

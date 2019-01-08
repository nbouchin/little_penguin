// SPDX-License-Identifier: GPL-2.0

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
#include <linux/semaphore.h>

MODULE_LICENSE("GPL");

static const char g_s_logname[] = "nbouchin";
static char g_s_chararray[8];
static const ssize_t g_s_logname_size = sizeof(g_s_logname);
static char ptr[PAGE_SIZE];
static size_t ldata;

DEFINE_SEMAPHORE(lock);

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
	if (*offp > 7 || count > 8)
		return -EINVAL;
	if (copy_from_user(g_s_chararray + *offp, buff, count) != 0)
		return -EFAULT;
	*offp += count;
	if (!strncmp(g_s_logname, g_s_chararray, strlen(buff)))
		pr_info("Device write is ok\n");
	else
		return -EINVAL;
	return count;
}

ssize_t foo_file_read(struct file *filp, char __user *buff, size_t count,
		      loff_t *offp)
{
	down(&lock);
	if (*offp >= PAGE_SIZE) {
		up(&lock);
		return 0;
	}
	if (*offp + count > ldata)
		count = ldata - *offp;
	if (copy_to_user(buff, ptr + *offp, count) != 0) {
		up(&lock);
		return -EFAULT;
	}
	*offp += count;
	up(&lock);
	return count;
}

ssize_t foo_file_write(struct file *filp, const char __user *buff, size_t count,
		       loff_t *offp)
{
	down(&lock);
	if (!buff) {
		up(&lock);
		return -EFAULT;
	}
	if (copy_from_user(ptr + ldata, buff, count) != 0) {
		up(&lock);
		return -EFAULT;
	}
	*offp += count;
	ldata += count;
	if (ldata > PAGE_SIZE) {
		memset(ptr, 0, PAGE_SIZE);
		*offp = 0;
		ldata = 0;
	}
	up(&lock);
	return count;
}

struct dentry *fortytwo;
const struct file_operations id_fops = { .read = id_file_read,
					 .write = id_file_write };
const struct file_operations foo_fops = { .read = foo_file_read,
					  .write = foo_file_write };

static int __init misc_init(void)
{
	fortytwo = debugfs_create_dir("fortytwo", NULL);
	debugfs_create_file("id", 0774, fortytwo, NULL, &id_fops);
	debugfs_create_u64("jiffies", 0444, fortytwo, (void *)&jiffies);
	debugfs_create_file("foo", 0644, fortytwo, NULL, &foo_fops);
	pr_info("Debugfs files generated.\n");
	return 0;
}

static void __exit misc_exit(void)
{
	debugfs_remove_recursive(fortytwo);
	pr_info("Cleaning up debugfs files.\n");
}

module_init(misc_init);
module_exit(misc_exit);

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
static char g_s_chararray[101];
static const ssize_t g_s_logname_size = sizeof(g_s_logname) - 1;
static char ptr[PAGE_SIZE];
size_t ldata;

DEFINE_SEMAPHORE(lock);

ssize_t id_file_read(struct file *filp, char __user *buff, size_t count,
		     loff_t *offp)
{
	return simple_read_from_buffer(buff, count, offp, g_s_logname, 8);
}

ssize_t id_file_write(struct file *filp, const char __user *buff, size_t count,
		      loff_t *offp)
{
	size_t ret;

	ret = simple_write_to_buffer(g_s_chararray, 100, offp, buff, count);
	if (ret < 0)
		return ret;
	if (!strncmp(g_s_logname, g_s_chararray, strlen(g_s_chararray))) {
		pr_info("Device write is ok\n");
		return count;
	}
	memset(g_s_chararray, 0, sizeof(g_s_chararray));
	return -EINVAL;
}

ssize_t foo_file_read(struct file *filp, char __user *buff, size_t count,
		      loff_t *offp)
{
	size_t ret;

	down(&lock);
	ret = simple_read_from_buffer(buff, count, offp, ptr, ldata);
	if (ret < 0) {
		up(&lock);
		return ret;
	}
	up(&lock);
	return ret;
}

ssize_t foo_file_write(struct file *filp, const char __user *buff, size_t count,
		       loff_t *offp)
{
	size_t ret;

	down(&lock);
REFRESH:
	if (count == 0) {
		up(&lock);
		return 0;
	}
	ret = simple_write_to_buffer(ptr, PAGE_SIZE, offp, buff, count);
	if (ret < 0) {
		up(&lock);
		return -EFAULT;
	} else if (ret < count) {
		memset(ptr, 0, PAGE_SIZE);
		*offp = 0;
		ldata = 0;
		goto REFRESH;
	}
	if (*offp > ldata)
		ldata = *offp;
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

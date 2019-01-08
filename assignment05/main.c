// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");

static const char g_s_logname[] = "nbouchin";
static char g_s_chararray[101];
static const ssize_t g_s_logname_size = sizeof(g_s_logname) - 1;

ssize_t device_file_read(struct file *filp, char __user *buff, size_t count,
			 loff_t *offp)
{
	return simple_read_from_buffer(buff, count, offp, g_s_logname, 8);
}

ssize_t device_file_write(struct file *filp, const char __user *buff,
			  size_t count, loff_t *offp)
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

const struct file_operations fops = { .owner = THIS_MODULE,
				      .read = device_file_read,
				      .write = device_file_write };

struct miscdevice misc = { .minor = MISC_DYNAMIC_MINOR,
			   .name = "fortytwo",
			   .fops = &fops };

static int __init misc_init(void)
{
	pr_info("Hello World !\n");
	misc_register(&misc);
	return 0;
}

static void __exit misc_exit(void)
{
	pr_info("Cleaning up module.\n");
	misc_deregister(&misc);
}

module_init(misc_init);
module_exit(misc_exit);

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");

static const char g_s_logname[] = "nbouchin";
static char g_s_chararray[9];
static const ssize_t g_s_logname_size = sizeof(g_s_logname);

ssize_t device_file_read(struct file *filp, char __user *buff, size_t count,
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

ssize_t device_file_write(struct file *filp, const char __user *buff,
			  size_t count, loff_t *offp)
{
	if (!buff || count != g_s_logname_size) {
		return -EFAULT;
	} else {
		if (copy_from_user(g_s_chararray + *offp, buff, count) != 0) {
			return -EFAULT;
		}
		*offp += count;
	}
	if (!strncmp(g_s_logname, g_s_chararray, 8)) {
		printk(KERN_INFO "Device write is ok\n");
	} else {
		return -EFAULT;
	}
	return count;
}

struct file_operations fops = { .read = device_file_read,
				.write = device_file_write };

struct miscdevice misc = { .minor = MISC_DYNAMIC_MINOR,
			   .name = "fortytwo",
			   .fops = &fops };

static int __init misc_init(void)
{
	printk(KERN_INFO "Hello World !\n");
	misc_register(&misc);
	return 0;
}

static void __exit misc_exit(void)
{
	printk(KERN_INFO "Cleaning up module.\n");
	misc_deregister(&misc);
}

module_init(misc_init);
module_exit(misc_exit);

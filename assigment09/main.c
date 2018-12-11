#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/mount.h>
#include <linux/fs_struct.h>
#include <linux/proc_fs.h>
#include </usr/src/linux-next-next-20181210/fs/proc/internal.h>

MODULE_LICENSE("GPL");

struct dentry *mymounts = NULL;
struct proc_dir_entry *proc_dir_entry = NULL;

ssize_t read_proc(struct file *filp, char *buf, size_t count, loff_t *offp)
{
	list_for_each_entry (mymounts, &current->fs->root.mnt->mnt_root->d_subdirs, d_child) {
		if (mymounts->d_flags & DCACHE_MOUNTED) {
			if (copy_to_user(buf, mymounts->d_name.name, count) != 0) {
				return -EFAULT;
			}
			*offp += count;
			printk("%s is mounted", mymounts->d_name.name);
		}
	}
	return count;
}

struct file_operations fops = { .read = read_proc };

static int __init mountlist_init(void)
{
	proc_dir_entry = proc_create("mymounts", 0777, NULL, &fops);
	printk(KERN_INFO "mountlist module loaded.\n");
	return 0;
}

static void __exit mountlist_exit(void)
{
	proc_remove(proc_dir_entry);
	printk(KERN_INFO "Cleaning up module.\n");
}

module_init(mountlist_init);
module_exit(mountlist_exit);

// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mount.h>
#include <linux/fs_struct.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/namei.h>
#include <linux/nsproxy.h>
#include <linux/dcache.h>
#include <../fs/proc/internal.h>
#include <../fs/mount.h>

MODULE_LICENSE("GPL");

#define PROC_NAME "mymounts"

static void *my_seq_start(struct seq_file *s, loff_t *pos)
{
	struct mnt_namespace *ns = current->nsproxy->mnt_ns;

	if (*pos != 0)
		return 0;
	s->private = list_first_entry(&ns->list, struct mount, mnt_list);
	return pos;
}

static void *my_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	loff_t *spos = v;
	struct mount *mount = s->private;

	mount = list_next_entry(mount, mnt_list);
	s->private = mount;

	if (&mount->mnt_list == &current->nsproxy->mnt_ns->list)
		return NULL;
	*pos += 1;
	return spos;
}

static void my_seq_stop(struct seq_file *s, void *v)
{
}

void list_backward(struct seq_file *s, struct mount *mnt)
{
	int i = 0;
	int lock = 0;
	char buff[256] = { 0 };
	char *table[128] = { 0 };
	struct mount *mnt_parent = NULL;

	mnt_parent = mnt->mnt_parent;
	seq_printf(s, "%-16s", mnt->mnt_devname);
	for (;
	     mnt_parent && strcmp(mnt_parent->mnt_mountpoint->d_name.name, "/");
	     i++) {
		table[i] = kstrdup(dentry_path_raw(mnt_parent->mnt_mp->m_dentry,
						   buff, 256),
				   GFP_KERNEL);
		mnt_parent = mnt_parent->mnt_parent;
	}
	for (i--; i >= 0; i--) {
		seq_printf(s, "%s", table[i]);
		kfree(table[i]);
		lock = 1;
	}
	if (!strcmp(dentry_path_raw(mnt->mnt_mp->m_dentry, buff, 256), "/") &&
	    lock) {
	} else {
		seq_printf(s, "%s",
			   dentry_path_raw(mnt->mnt_mp->m_dentry, buff, 256));
	}
	seq_puts(s, "\n");
}

static int my_seq_show(struct seq_file *s, void *v)
{
	struct mount *mnt = s->private;

	if (mnt->mnt_mountpoint &&
	    mnt->mnt_mountpoint->d_flags & DCACHE_MOUNTED && mnt->mnt_mp) {
		list_backward(s, mnt);
	}
	return 0;
}

static const struct seq_operations my_seq_ops = { .start = my_seq_start,
						  .next = my_seq_next,
						  .stop = my_seq_stop,
						  .show = my_seq_show };

static int my_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &my_seq_ops);
};

static const struct file_operations my_file_ops = { .owner = THIS_MODULE,
						    .open = my_open,
						    .read = seq_read,
						    .llseek = seq_lseek,
						    .release = seq_release };

int init_module(void)
{
	struct proc_dir_entry *entry;

	entry = proc_create(PROC_NAME, 0, NULL, &my_file_ops);
	if (entry < 0)
		return -1;
	return 0;
}

void cleanup_module(void)
{
	remove_proc_entry(PROC_NAME, NULL);
}

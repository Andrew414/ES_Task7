#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/timer.h>
#include <linux/init.h>

/* Default timer tact period */
atomic_t tact = ATOMIC_INIT(5000);

/* Timer structure */
static struct timer_list sos_timer;

/* Kobject structure */
static struct kobject * kobj;

/* Shows the current timer frequency in milliseconds. */
static ssize_t file_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);

/* Sets the current timer frequency in milliseconds. */
static ssize_t file_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);

/* Kobject attribute */
static struct kobj_attribute sos_attr =
  __ATTR(sos, 0666,
  file_show, file_store);

/* Attrs structures array */
static struct attribute *attrs[] = {
	&sos_attr.attr, NULL
};

/* Attributes group. */
static struct attribute_group attr_group = {
	.attrs = attrs,
};

/* Timer callback being invoked every tact */
void sos_timer_callback(unsigned long data)
{
	int c = atomic_read(&tact);
	if (c != -1) {
		printk(KERN_INFO " --- SOS --- | Next will be in %d msec\n", c);
		mod_timer(&sos_timer, jiffies + msecs_to_jiffies(c));
	}
}

/* Shows the current timer frequency in milliseconds. */
static ssize_t file_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "Timer frequency is %d msecs now.\n", atomic_read(&tact));
}

/* Sets the current timer frequency in milliseconds. */
static ssize_t file_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	int res, cache = 0;
	res = sscanf(buf, "%du", &cache);
	if (res <= 0) {
		printk(KERN_ERR "Value is not appliable: %s!\n", buf);
		return count;
	}
	if (cache > 500) {
		atomic_set(&tact, cache);
	} else {
		printk(KERN_ERR "%d value is invalid.\n", cache);
	}
	return count;
}

/* Module init function */
static int __init timer_init(void)
{
	int ret, start = atomic_read(&tact);
	printk(KERN_INFO "Timer module is installing.\n");
	setup_timer(&sos_timer, sos_timer_callback, 0);
	printk(KERN_INFO "Starting timer to fire in default %dms (jiffies %ld)\n", start, jiffies);
	ret = mod_timer(&sos_timer, jiffies + msecs_to_jiffies(start));
	if (ret) {
		printk(KERN_INFO "Error when setting up timer.\n");
	}

	kobj = kobject_create_and_add("timerk", kernel_kobj);
	if (!kobj) {
		return -ENOMEM;
	}
	ret = sysfs_create_group(kobj, &attr_group);
	if (ret) {
		kobject_put(kobj);
	}
	return ret;
}

/* Module exit function */
static void __exit timer_exit(void)
{
	int ret;
	atomic_set(&tact, -1);
	ret = del_timer(&sos_timer);
	if (ret) {
		printk(KERN_ERR "Failed to turn off timer.\n");
	}
	kobject_put(kobj);
	printk(KERN_INFO "Timer module uninstalled.\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("AnastasiyaKoloskova && AndrewFedchuk");

module_init(timer_init); /* Register module entry point */
module_exit(timer_exit); /* Register module cleaning up */

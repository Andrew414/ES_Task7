#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/timer.h>
#include <linux/init.h>
#include "timer.h"

#define DEFAULT_TICK 1000
#define MIN_TIMER 500
#define FILE_ATTRIBUTES 0666

typedef struct timer_list TIMER_LIST;
typedef struct kobject KOBJECT, *PKOBJECT;
typedef struct kobj_attribute KOBJ_ATTRIBUTE, *PKOBJ_ATTRIBUTE;
typedef struct attribute  ATTRIBUTE, *PATTRIBUTE;
typedef struct attribute_group ATTRIBUTE_GROUP;

/* Default timer tact period */
atomic_t tact = ATOMIC_INIT(DEFAULT_TICK);

/* Timer structure */
static TIMER_LIST sos_timer;

/* Kobject structure */
static PKOBJECT kobj;

/* Shows the current timer frequency in milliseconds. */
static ssize_t file_show(PKOBJECT kobj, PKOBJ_ATTRIBUTE attr, char *buf);

/* Sets the current timer frequency in milliseconds. */
static ssize_t file_store(PKOBJECT kobj, PKOBJ_ATTRIBUTE attr, const char *buf, size_t count);

/* Kobject attribute */
static KOBJ_ATTRIBUTE  sos_attr =
  __ATTR(period, FILE_ATTRIBUTES,
	file_show, file_store);

/* Attrs structures array */
static PATTRIBUTE attrs[] = {
	&sos_attr.attr, NULL
};

/* Attributes group. */
static ATTRIBUTE_GROUP attr_group = {
	.attrs = attrs,
};

/* Timer callback being invoked every tact */
void sos_timer_callback(unsigned long data)
{
	int c = atomic_read(&tact);
	if (c != -1) {
		printk(KERN_INFO MODULE_PREFIX " ...---... | SOS\n");
		mod_timer(&sos_timer, jiffies + msecs_to_jiffies(c));
	}
}

/* Shows the current timer frequency in milliseconds. */
static ssize_t file_show(PKOBJECT kobj, PKOBJ_ATTRIBUTE attr, char *buf)
{
	return sprintf(buf, "%d\n", atomic_read(&tact));
}

/* Sets the current timer frequency in milliseconds. */
static ssize_t file_store(PKOBJECT kobj, PKOBJ_ATTRIBUTE attr, const char *buf, size_t count)
{
	int res, cache = 0;
	res = sscanf(buf, "%du", &cache);
	if (res <= 0) {
		printk(KERN_ERR MODULE_PREFIX "Value is not appliable: %s!\n", buf);
		return count;
	}
	if (cache > MIN_TIMER) {
		atomic_set(&tact, cache);
		printk(KERN_INFO MODULE_PREFIX "Timer period set to %d ms\n", cache);
	} else {
		printk(KERN_ERR MODULE_PREFIX "%d value is invalid.\n", cache);
	}
	return count;
}

/* Module init function */
static int __init timer_init(void)
{
	int ret, start = atomic_read(&tact);
	printk(KERN_INFO MODULE_PREFIX "Timer module installed.\n");
	printk(KERN_INFO MODULE_PREFIX "Timer period can be set and seen in \"/sys/kernel/sostimer/period\"");
	setup_timer(&sos_timer, sos_timer_callback, 0);
	printk(KERN_INFO MODULE_PREFIX "Starting timer with %d ms period\n", start);
	ret = mod_timer(&sos_timer, jiffies + msecs_to_jiffies(start));
	if (ret) {
		printk(KERN_INFO MODULE_PREFIX "Error when setting up timer.\n");
	}

	kobj = kobject_create_and_add("sostimer", kernel_kobj);
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
	kobject_put(kobj);
	printk(KERN_INFO MODULE_PREFIX "Timer module uninstalled.\n");
}

module_init(timer_init); /* Register module entry point */
module_exit(timer_exit); /* Register module cleaning up */

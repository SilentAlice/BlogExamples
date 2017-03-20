/* Demo: PV Split Driver
 * Post: http://silentming.net/blog/2017/03/20/xen-log-14-pv-driver/
 * This is kernel module under GPL License * Environment: Debian 8, Linux 4.10.2, Xen 4.5.1
 *
 * Compile:
 * make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
 *
 * Run:
 * insmod alice_dom0.ko
 *
 * This Module is running in dom0 acting as backend
 */
#include <linux/module.h> 

#include <xen/xen.h>
#include <xen/xenbus.h>

/* The function is called on activation of the device */
static int alice_back_probe(struct xenbus_device *dev,
			const struct xenbus_device_id *id)
{
	pr_info("Dom0: Probe called.\n");
	return 0;
}

/* This defines the name of the devices the driver reacts to */
static const struct xenbus_device_id alice_back_ids[] = {
	{ "alice_dev" },
	{ "" }
};

/* We set up the callback functions */
static struct xenbus_driver alice_back_driver = {
	.ids  = alice_back_ids,
	.probe = alice_back_probe,
};

/* On loading this kernel module, we register as a backend driver */
static int __init init_alice(void)
{
	pr_info("Dom0: Alice_back inited!\n");
	return xenbus_register_backend(&alice_back_driver);
}

/* unregister when rmmod */
static void __exit exit_alice(void)
{
	xenbus_unregister_driver(&alice_back_driver);
	printk(KERN_ALERT "Dom0: Alice Exit Successfully.\n");
}
module_init(init_alice);
module_exit(exit_alice);

MODULE_LICENSE("GPL");

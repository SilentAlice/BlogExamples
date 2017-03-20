/* Demo: PV Split Driver
 * Post: http://silentming.net/blog/2017/03/20/xen-log-14-pv-driver/
 * This is kernel module under GPL License * Environment: Debian 8, Linux 4.10.2, Xen 4.5.1
 *
 * Compile:
 * make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
 *
 * Run:
 * insmod alice_domU.ko
 *
 * This Module is running in domU acting as frontend
 */
#include <linux/module.h>  /* Needed by all modules */

#include <xen/xen.h>
#include <xen/xenbus.h>

/* The function is called on activation of the device */
static int alice_front_probe(struct xenbus_device *dev,
              const struct xenbus_device_id *id)
{
	pr_info("DomU: Probe called.\n");
	return 0;
}

/* This defines the name of the devices the driver reacts to
 * So, this should be consistent with backend */
static const struct xenbus_device_id alice_front_ids[] = {
	{ "alice_dev" },
	{ ""  }
};

/* We set up the callback functions */
static struct xenbus_driver alice_front_driver = {
	.ids  = alice_front_ids,
	.probe = alice_front_probe,
};

/* On loading this kernel module, we register as a frontend driver */
static int __init init_alice(void)
{
	pr_info("DomU: Alice_front inited!\n");
	return xenbus_register_frontend(&alice_front_driver);
}

/* unregister when rmmod */
static void __exit exit_alice(void)
{
	xenbus_unregister_driver(&alice_front_driver);
	pr_info("DomU: Alice Exit Successfully\n");
}
module_init(init_alice);
module_exit(exit_alice);

MODULE_LICENSE("GPL");

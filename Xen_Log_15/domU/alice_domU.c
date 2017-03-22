/* Demo: PV Split Driver
 * Post: http://silentming.net/blog/2017/03/21/xen-log-15-xenbus/
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

/* This is where we set up xenstore files and event channels */
static int alice_front_connect(struct xenbus_device *dev)
{
	pr_info("DomU: Connecting the frontend now\n");
	return 0;
}

/* The function is called on a state change of the backend driver */
static void alice_front_otherend_changed(struct xenbus_device *dev,
			    enum xenbus_state backend_state)
{
	switch (backend_state)
	{
		case XenbusStateInitialising:
			xenbus_switch_state(dev, XenbusStateInitialising);
			break;
		case XenbusStateInitialised:
		case XenbusStateReconfiguring:
		case XenbusStateReconfigured:
		case XenbusStateUnknown:
			break;

		case XenbusStateInitWait:
			if (dev->state != XenbusStateInitialising)
				break;
            /* Connect */
			if (alice_front_connect(dev) != 0)
				break;

			xenbus_switch_state(dev, XenbusStateConnected);

			break;

		case XenbusStateConnected:
			pr_info("DomU: Other side says it is connected as well.\n");
			break;

		case XenbusStateClosed:
			if (dev->state == XenbusStateClosed)
				break;
			/* Missed the backend's CLOSING state -- fallthrough */
		case XenbusStateClosing:
			xenbus_frontend_closed(dev);
	}
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
    .otherend_changed = alice_front_otherend_changed,
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

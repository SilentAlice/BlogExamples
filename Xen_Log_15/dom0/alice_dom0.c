/* Demo: PV Split Driver
 * Post: http://silentming.net/blog/2017/03/21/xen-log-15-xenbus/
 * This is kernel module under GPL License * Environment: Debian 8, Linux 4.10.2, Xen 4.5.1
 *
 * Compile:
 * make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
 *
 * Run:
 * insmod alice_dom0.ko
 *
 * This Module is running in dom0 acting as backend
 * After insmod domU, use activate to issue communication
 */
#include <linux/module.h>  /* Needed by all modules */

#include <xen/xen.h>
#include <xen/xenbus.h>


/* This is where we set up path watchers and event channels */
static void alice_back_connect(struct xenbus_device *dev)
{ pr_info("Dom0: Connect the backend\n"); }

/* Disconnect from xenbus, do clean work (event channel etc) */
static void alice_back_disconnect(struct xenbus_device *dev)
{ pr_info("Dom0: Disconnect the backend\n"); }

/* We try to switch to the next state from a previous one */
static void set_backend_state(struct xenbus_device *dev,
			      enum xenbus_state state)
{
	while (dev->state != state) {
		switch (dev->state) {
		case XenbusStateInitialising:
			switch (state) {
			case XenbusStateInitWait:
			case XenbusStateConnected:
			case XenbusStateClosing:
				xenbus_switch_state(dev, XenbusStateInitWait);
				break;
			case XenbusStateClosed:
				xenbus_switch_state(dev, XenbusStateClosed);
				break;
			default:
				BUG();
			}
			break;
		case XenbusStateClosed:
			switch (state) {
			case XenbusStateInitWait:
			case XenbusStateConnected:
				xenbus_switch_state(dev, XenbusStateInitWait);
				break;
			case XenbusStateClosing:
				xenbus_switch_state(dev, XenbusStateClosing);
				break;
			default:
				BUG();
			}
			break;
		case XenbusStateInitWait:
			switch (state) {
			case XenbusStateConnected:
				alice_back_connect(dev);
				xenbus_switch_state(dev, XenbusStateConnected);
				break;
			case XenbusStateClosing:
			case XenbusStateClosed:
				xenbus_switch_state(dev, XenbusStateClosing);
				break;
			default:
				BUG();
			}
			break;
		case XenbusStateConnected:
			switch (state) {
			case XenbusStateInitWait:
			case XenbusStateClosing:
			case XenbusStateClosed:
				alice_back_disconnect(dev);
				xenbus_switch_state(dev, XenbusStateClosing);
				break;
			default:
				BUG();
			}
			break;
		case XenbusStateClosing:
			switch (state) {
			case XenbusStateInitWait:
			case XenbusStateConnected:
			case XenbusStateClosed:
				xenbus_switch_state(dev, XenbusStateClosed);
				break;
			default:
				BUG();
			}
			break;
		default:
			BUG();
		}
	}
}

/* The function is called on activation of the device */
static int alice_back_probe(struct xenbus_device *dev,
			const struct xenbus_device_id *id)
{
	pr_info("Dom0: Probe called.\n");
    xenbus_switch_state(dev, XenbusStateInitialising);
	return 0;
}

/* The function is called on a state change of the frontend driver */
static void alice_back_otherend_changed(struct xenbus_device *dev, enum xenbus_state frontend_state)
{
	switch (frontend_state) {
		case XenbusStateInitialising:
			set_backend_state(dev, XenbusStateInitWait);
			break;

		case XenbusStateInitialised:
			break;

		case XenbusStateConnected:
			set_backend_state(dev, XenbusStateConnected);
			break;

		case XenbusStateClosing:
			set_backend_state(dev, XenbusStateClosing);
			break;

		case XenbusStateClosed:
			set_backend_state(dev, XenbusStateClosed);
			if (xenbus_dev_is_online(dev))
				break;
			/* fall through if not online */
		case XenbusStateUnknown:
			set_backend_state(dev, XenbusStateClosed);
			device_unregister(&dev->dev);
			break;

		default:
			xenbus_dev_fatal(dev, -EINVAL, "saw state %s (%d) at frontend",
					xenbus_strstate(frontend_state), frontend_state);
			break;
	}
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
	.otherend_changed = alice_back_otherend_changed,
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
	pr_info("Dom0: Alice Exit Successfully.\n");
}

module_init(init_alice);
module_exit(exit_alice);

MODULE_LICENSE("GPL");

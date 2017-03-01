/* Demo: Event Channel 
 * Post: http://silentming.net/blog/2017/03/01/xen-log-12-using-event-channel/
 * This is kernel module under GPL License
 * Environment: Debian 8u2, Linux 3.16.0-4-amd64, Xen 4.5.1
 *
 * Compile:
 * make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
 *
 * Run:
 * insmod alice_domU.ko
 *
 * This Module is running in domU before dom0 module to communicate with dom0
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/gfp.h>
#include <linux/proc_fs.h>

#include <asm/xen/page.h>
#include <xen/grant_table.h>
#include <xen/events.h>

#include <xen/interface/io/ring.h>
#include <xen/interface/xen.h>


#define DOM0_ID 0

typedef struct info {
    int irq;
    int evtchn;
} info_t;

info_t global_info;

static irqreturn_t domU_handler(int irq, void *dev_id)
{
    info_t *info = (info_t *)dev_id;
    pr_info("DomU: Handled! irq: %d, evtchn:%d\n", irq, info->evtchn);
    /* After domU handle irq, notify dom0 */
    notify_remote_via_irq(irq);
    return IRQ_HANDLED;
}


static int init_alice(void)
{

    struct evtchn_alloc_unbound alloc_unbound;
    int err;

    alloc_unbound.dom = DOMID_SELF;
    alloc_unbound.remote_dom = DOM0_ID;
    
    /* Get a unbound evtchn */
    err = HYPERVISOR_event_channel_op(EVTCHNOP_alloc_unbound, &alloc_unbound);

    if ( err < 0 ) {
        pr_err("DomU: Can't alloc unbound evtchn, err:%d\n", err);
        return 0;
    } else {
        global_info.evtchn = alloc_unbound.port;
        pr_info("DomU: Get new evtchn: %d\n", global_info.evtchn);
    }
    err = bind_evtchn_to_irq(global_info.evtchn);
    pr_info("DomU: Bound local irq: %d to evtchn:%d\n", err, global_info.evtchn);
    global_info.irq = err;

    err = request_irq(global_info.irq, domU_handler, 0, "alice_dev", &global_info);
    if ( err != 0 ) {
        pr_err("DomU: Cant bound to handler\n");
        unbind_from_irqhandler(global_info.irq, "alice_dev");
        return 0;
    }

    return 0;
}

static void exit_alice(void)
{
    unbind_from_irqhandler(global_info.irq, "alice_dev");
    pr_info("DomU: Exit Successfully\n");
    return ;
}

module_init(init_alice);
module_exit(exit_alice);

MODULE_LICENSE("GPL");




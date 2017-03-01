/* Demo: Event Channel 
 * Post: http://silentming.net/blog/2017/03/01/xen-log-12-using-event-channel/
 * This is kernel module under GPL License
 * Environment: Debian 8u2, Linux 3.16.0-4-amd64, Xen 4.5.1
 *
 * Compile:
 * make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
 *
 * Run:
 * insmod alice_dom0.ko domid=<domid> port=<evtchn>
 *
 * <domid>  domID of remote domU
 *
 * <evtchn> evtchn allocated by remote domU
 *
 * This Module is running in dom0 after domU module to communicate with domU
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>

#include <xen/grant_table.h>
#include <asm/xen/hypercall.h>

#include <xen/interface/grant_table.h>
#include <xen/interface/io/ring.h>

#include <xen/events.h>

typedef struct info {
    int irq;
    int evtchn;
    int remoteDomID;
} info_t;

info_t global_info;
int domid;
int port;

module_param(domid, int, 0644);
module_param(port, int, 0644);

static irqreturn_t dom0_handler(int irq, void *dev_id)
{
    /* Usually, dev_id is address of info struct, 
     * so this handler can make use of this */
    info_t *info = dev_id;
    pr_info("Dom0: Handled, domid: %d, port: %d, local_irq:%d\n", 
            info->remoteDomID, info->evtchn, irq);
    /* Revice event from domU */
    return IRQ_HANDLED;
}

int init_alice(void)
{
    int err;
    
    global_info.remoteDomID = domid;
    global_info.evtchn = port;

    pr_info("Dom0: init info with remoteDomID:%d, port:%d\n", domid, port);

    err = bind_interdomain_evtchn_to_irqhandler(global_info.remoteDomID,
            global_info.evtchn, dom0_handler, 0, "alice_dev", &global_info);

    global_info.irq = err;
    if ( err > 0 ) {
        pr_info("Dom0: bound local irq:%d to evtchn:%d\n", err, global_info.evtchn);
    }

    notify_remote_via_irq(global_info.irq);
    return 0;
}

void exit_alice(void)
{
    unbind_from_irqhandler(global_info.irq, &global_info);
    pr_info("Dom0: Exit Successfully\n");
}

module_init(init_alice);
module_exit(exit_alice);

MODULE_LICENSE("GPL");

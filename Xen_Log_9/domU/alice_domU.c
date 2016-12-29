/* Demo: I/O Ring
 * Post: http://silentming.net/blog/2016/12/28/xen-log-9-io-ring/
 * This is kernel module under GPL License
 * Environment: Debian 8u2, Linux 3.16.0-4-amd64, Xen 4.5.1
 *
 * Compile:
 * make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
 *
 * Run this in domU first:
 * insmod alice_domU.ko
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/gfp.h>
#include <linux/proc_fs.h>

#include <asm/xen/page.h>
#include <xen/grant_table.h>

#include <xen/interface/io/ring.h>
#include <xen/interface/xen.h>

#define DOM0_ID 0

/* Ring request & respond, used by DEFINE_RING_TYPES macro */
struct as_request {
    int hello;
};

struct as_response {
    int hi;
};

/* this macro will create as_sring, as_back_ring, as_front_ring */
DEFINE_RING_TYPES(as, struct as_request, struct as_response);

typedef struct front_end_t {
    struct as_front_ring ring;  /* Record real ring */
    grant_ref_t gref;           /* gref of shared page */
} front_end_t;

front_end_t front_end;

void send_request(int hello)
{
    struct as_request *ring_req;
    int notify;
    /* Write a request and update the req-prod pointer */
    ring_req = RING_GET_REQUEST(&(front_end.ring), front_end.ring.req_prod_pvt);
    ring_req->hello = hello;
    front_end.ring.req_prod_pvt += 1;

    RING_PUSH_REQUESTS_AND_CHECK_NOTIFY(&(front_end.ring), notify);

    if ( notify ) {
        pr_info("Alice: Need notify");
    }
}

static int init_alice(void)
{
    unsigned long mfn;
    unsigned long vpage;
    struct as_sring *sring;
    int gref;

    pr_info("Alice: Hello, This is Alice\n");

    /* Step 1: Alloc page for ring */ 
    vpage = __get_free_pages(GFP_KERNEL, 1);
    if ( vpage == 0 ) {
        pr_err("Alice: Could not get free pages\n");
        return 0;
    }
    pr_info("Alice: Get free pages from kernel, virt of page: 0x%lx\n", vpage);

    /* Step 2: Put shared ring on this page to be shared */
    sring = (struct as_sring *)vpage;
    SHARED_RING_INIT(sring);

    /* Step 3: Front init */
    FRONT_RING_INIT(&(front_end.ring), sring, PAGE_SIZE);

    /* Step 4: Share this ring with dom0, readonly */
    mfn = virt_to_mfn(vpage);
    gref = gnttab_grant_foreign_access(DOM0_ID, mfn, 0);
    if ( gref < 0 ) {
        pr_err("Alice: Could not grant foreign access");
        return 0;
    }
    front_end.gref = gref;
    pr_info("Alice: Grant_Ref is %d, input this as param of alice_dom0.ko\n", gref);

    /* Step 5: fill content, and send this by request */
    send_request(233);

    return 0;
}

static void exit_alice(void)
{
    RING_IDX rc, rp;
    struct as_response *rsp;
    
    rc = front_end.ring.rsp_cons;
    rp = front_end.ring.sring->rsp_prod;
    rsp = RING_GET_RESPONSE(&front_end.ring, rc); 

    pr_info("Alice: Get response, hi = %d\n", rsp->hi);
    front_end.ring.rsp_cons = ++rc;

    pr_info("Alice: Cleanup grant ref...\n");
    if ( gnttab_query_foreign_access(front_end.gref) == 0 ) {
        pr_info("Alice: No one is mapping this ref\n");
        gnttab_end_foreign_access(front_end.gref, 0, (unsigned long)front_end.ring.sring);
    } else {
        pr_info("Alice: Someone is mapping this ref now \n");
        /* This is exit, we still have to remove this... */
        gnttab_end_foreign_access(front_end.gref, 0, (unsigned long)front_end.ring.sring);
    }

    /* end_foreign_access will free page */
    //free_pages(vpage, 1);

    pr_info("Alice: Exit Successfully\n");
    return ;
}

module_init(init_alice);
module_exit(exit_alice);

MODULE_LICENSE("GPL");




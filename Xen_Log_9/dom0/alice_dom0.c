/* Demo: I/O Ring 
 * Post: http://silentming.net/blog/2016/12/28/xen-log-9-io-ring/
 * This is kernel module under GPL License
 * Environment: Debian 8u2, Linux 3.16.0-4-amd64, Xen 4.5.1
 *
 * Compile:
 * make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
 *
 * Run:
 * insmod alice_dom0.ko gref=<value> domid=<domid>
 *
 * <value>  Taken from dmesg output in alice_domU when you insmod
 *          alice_domU in domU. grant ref of shared ring.
 * <domid>  domID of remote domU
 *
 * This Module is running in dom0 to read info from domU
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/vmalloc.h>

#include <xen/grant_table.h>
#include <asm/xen/hypercall.h>

#include <xen/interface/grant_table.h>
#include <xen/interface/io/ring.h>

struct as_request {
    int hello;
};
struct as_response {
    int hi;
};

/* Define as_sring, as_back_ring, as_front_ring type */
DEFINE_RING_TYPES(as, struct as_request, struct as_response);
typedef struct as_sring as_sring_t;
typedef struct as_request as_request_t;
typedef struct as_response as_response_t;

typedef struct back_end_t {
    struct as_back_ring ring;  /* Record real ring */
    grant_ref_t gref;          /* gref of sring */
    int domid;
} back_end_t;

struct gnttab_map_grant_ref ops;
struct gnttab_unmap_grant_ref unmap_ops;
back_end_t back_end;

int gref;
int domid;

module_param(gref, int, 0644);
module_param(domid, int, 0644);

void handle_request(void)
{
    RING_IDX rc, rp; 
    as_request_t req;
    as_response_t rsp;
    int notify;

    rc = back_end.ring.req_cons;
    rp = back_end.ring.sring->req_prod;
    pr_info("req_comsumer: %d, req_producer: %d\n", rc, rp);

    /* Copy this info local */
    memcpy(&req, RING_GET_REQUEST(&back_end.ring, rc), sizeof(req));
    pr_info("Alice: Receive hello=%d\n", req.hello);

    /* Fill response hi = hello + 1 */
    rsp.hi = req.hello + 1;

    /* update req-consumer */
    back_end.ring.req_cons = ++rc;
    barrier();
    memcpy(RING_GET_RESPONSE(&back_end.ring, back_end.ring.rsp_prod_pvt),
            &rsp, sizeof(rsp));
    back_end.ring.rsp_prod_pvt++;

    RING_PUSH_RESPONSES_AND_CHECK_NOTIFY(&back_end.ring, notify);
    /* TODO: Add final checks */

    if ( notify ) {
        pr_info("Alice: Need to send notify to dom%d\n", back_end.domid);
    }

}

int init_alice(void)
{
    struct vm_struct *v_start;
    as_sring_t *sring;
    back_end.domid = domid;
    back_end.gref = gref;
    pr_info("Alice: init_module with gref = %d, domid = %d\n", back_end.gref, back_end.domid);

    /* Reserve a range of kernel address space, fill page table to map this range 
     * This PAGE_SIZE is used for map granted page */
    v_start = alloc_vm_area(PAGE_SIZE, NULL);
    if ( v_start == 0 ) {
        free_vm_area(v_start);
        pr_err("Alice: could not allocate page area\n");
        return 0;
    }

    /* Init map ops */
    gnttab_set_map_op(&ops, (unsigned long)v_start->addr, GNTMAP_host_map,
            back_end.gref, back_end.domid);
    if ( HYPERVISOR_grant_table_op(GNTTABOP_map_grant_ref, &ops, 1) ) {
        pr_err("Alice: HYPERVISOR map grant ref failed\n");
        return 0;
    }
    if ( ops.status ) {
        pr_err("Alice: HYPERVISOR map grant ref failed, status = %d\n", ops.status);
        return 0;
    }
    pr_info("Alice: shared_ring = %lx, handle = %x, status = %x\n",
            (unsigned long)v_start->addr, ops.handle, ops.status);

    sring = (as_sring_t *)v_start->addr;
    BACK_RING_INIT(&back_end.ring, sring, PAGE_SIZE);

    handle_request();

    /* Prepare for unmap */
    unmap_ops.host_addr = (unsigned long)(v_start->addr);
    unmap_ops.handle = ops.handle;
    return 0;
}

void exit_alice(void)
{
    pr_info("Alice: cleanup_module\n");
    if ( HYPERVISOR_grant_table_op(GNTTABOP_unmap_grant_ref, &unmap_ops, 1) ) {
        pr_err("Alice: unmap shared page failed\n");
    } else {
        pr_info("Alice: unmap shared page successfully\n");
    }
}

module_init(init_alice);
module_exit(exit_alice);

MODULE_LICENSE("GPL");

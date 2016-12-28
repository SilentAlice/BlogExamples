/* Demo: sharing memory using grant_table 
 * Post: http://silentming.net/blog/2016/12/26/xen-log-8-grant-table/
 * This is kernel module under GPL License
 *
 * Compile:
 * make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
 *
 * Run:
 * insmod alice_dom0.ko gref=<value> domid=<domid>
 *
 * <value>  Taken from dmesg output in alice_domU when you insmod 
 *          alice_domU in domU.
 * <domid>  domID of remote domU
 *
 * This Module is running in dom0 to read info from domU
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/vmalloc.h>

#include <xen/grant_table.h>
#include <xen/interface/grant_table.h>
#include <asm/xen/hypercall.h>

struct gnttab_map_grant_ref ops;
struct gnttab_unmap_grant_ref unmap_ops;

typedef struct info_t {
    grant_ref_t gref;
    int domid;  /* remote domID */
} info_t;

info_t info;
int gref;
int domid;

module_param(gref, int, 0644);
module_param(domid, int, 0644);

int init_alice(void)
{
    struct vm_struct *v_start;
    info.gref = gref;
    info.domid = domid;
    pr_info("Alice: init_module with gref = %d, domid = %d\n", info.gref, info.domid);

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
            info.gref, info.domid);
    if ( HYPERVISOR_grant_table_op(GNTTABOP_map_grant_ref, &ops, 1) ) {
        pr_err("Alice: HYPERVISOR map grant ref failed\n");
        return 0;
    }
    if ( ops.status ) {
        pr_err("Alice: HYPERVISOR map grant ref failed, status = %d\n", ops.status);
        return 0;
    }
    pr_info("Alice: shared_page = %lx, handle = %x, status = %x\n",
            (unsigned long)v_start->addr, ops.handle, ops.status);

    pr_info("Alice: info from domU: %s\n", (char *)(v_start->addr));

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

/* Demo: sharing memory using grant_table 
 * Post: http://silentming.net/blog/2016/12/26/xen-log-8-grant-table/
 * This is kernel module under GPL License
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

#include <asm/xen/page.h>

#include <xen/interface/xen.h>
#include <xen/grant_table.h>

#define DOM0_ID 0
unsigned long vpage;
unsigned long mfn;
int gref;

static int init_alice(void)
{
    /* Step 1: Get a page to be shared with dom0 */ 
    pr_info("--------->Hello, This is Alice\n");
    vpage = __get_free_pages(GFP_KERNEL, 1);
    if ( vpage == 0 ) {
        pr_err("Alice: Could not get free pages\n");
        return 0;
    }
    pr_info("Alice: Get free page from kernel, virt: 0x%lx\n", vpage);

    mfn = virt_to_mfn(vpage);

    gref = gnttab_grant_foreign_access(DOM0_ID, mfn, 0);
    if ( gref < 0 ) {
        pr_err("Alice: Could not grant foreign access");
        return 0;
    }

    pr_info("Alice: Grant_Ref is %d, input this as alice_dom0.ko param\n", gref);

    /* Step 2: Write some contents */
    strcpy((char *)vpage, "Hello, by Alice in domU\n");

    return 0;
}

static void exit_alice(void)
{
    pr_info("Cleanup grant ref...\n");

    if ( gnttab_query_foreign_access(gref) == 0 ) {
        pr_info("Alice: No one is mapping this ref\n");
        gnttab_end_foreign_access(gref, 0, vpage);
    } else {
        pr_info("Alice: Someone is mapping this ref now \n");
        /* This is exit, we still have to remove this... */
        gnttab_end_foreign_access(gref, 0, vpage);
    }

    /* end_foreign_access will free page */
    //free_pages(vpage, 1);

    pr_info("Alice: Exit Successfully\n");
    return ;
}

module_init(init_alice);
module_exit(exit_alice);

MODULE_LICENSE("GPL");




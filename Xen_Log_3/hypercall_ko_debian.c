#include <linux/module.h>
#include <linux/kernel.h>

#include <xen/interface/xen.h>
#include <asm/xen/hypercall.h>
/*
#define __HYPERVISOR_alice_op 39
static inline long
HYPERVISOR_alice_op(void)
{
	return _hypercall0(long, alice_op);
}
*/

static int init_hypercall(void)
{
	HYPERVISOR_alice_op();	
	printk("test\n");
	return 0;
}


static void exit_hypercall(void)
{
	
}

module_init(init_hypercall);
module_exit(exit_hypercall);

MODULE_LICENSE("GPL");

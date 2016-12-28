/* Demo: Kernel Module 
 * Post: http://silentming.net/blog/2016/12/28/kernel-module/
 * This kernel module is under GPL License
 *
 * Compile:
 * make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
 *
 * Run:
 * insmod alice_module.ko
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>

static int init_alice(void)
{
    
    pr_info("--------->Hello, This is Alice\n");
    return 0;
}

static void exit_alice(void)
{
   pr_info("================================>Exit Successfully\n");
}
module_init(init_alice);
module_exit(exit_alice);

MODULE_LICENSE("GPL");




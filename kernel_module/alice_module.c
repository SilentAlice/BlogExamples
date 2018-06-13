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

#define HASPARAM 1

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#if HASPARAM
#include <linux/moduleparam.h>

static int arg1;
static int arg2;
#endif

static int init_alice(void)
{
    
    pr_info("--------->Hello, This is Alice\n");
#if HASPARAM
    pr_info("param arg1:%d, arg2:%d\n", arg1, arg2);
#endif

    return 0;

}

static void exit_alice(void)
{
   pr_info("================================>Exit Successfully\n");
}

module_init(init_alice);
module_exit(exit_alice);

#if HASPARAM
module_param(arg1, int, 0644); 
module_param(arg2, int, 0644); 
#endif

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SilentAlice");
MODULE_DESCRIPTION("Simple module demo");




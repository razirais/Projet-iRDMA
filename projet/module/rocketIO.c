#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>

// To use symbolic values of error
#include <linux/errno.h>

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Adrien Oliva, Gaetan Harter");

#define DEBUG

#ifdef DEBUG
#	define DLOG(fmt, args...) printk("%s:"fmt"\n", __func__, ##args)
#	define KLOG(fmt, args...) printk(KERN_ERR fmt"\n", ##args)
	int debug = 1;
#else
#	define DLOG(fmt, args...)
#	define KLOG(fmt, args...)
	int debug = 0;
#endif




static int __init initialization_function(void)
{
        /* Initialization code here */


        return 0;
}

module_init(initialization_function);


static void __exit cleanup_function(void)
{
        /* Clean up code here */

}

module_exit(cleanup_function);


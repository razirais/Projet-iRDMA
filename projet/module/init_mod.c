#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>

// To use symbolic values of error
#include <linux/errno.h>

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Adrien Oliva, Gaetan Harter");

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


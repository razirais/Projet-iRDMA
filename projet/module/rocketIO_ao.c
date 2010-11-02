#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>

// Valeurs symbolic des erreurs
#include <linux/errno.h>

#define DEBUG

MODULE_AUTHOR("Geaetan Harter - Adrien Oliva");
MODULE_LICENSE("Dual BSD/GPL");

#ifdef DEBUG
#       define DLOG(fmt, args...) printk(KERN_DEBUG "ROCKET_IO %s: "fmt"\n", __func__, ##args)
#       define KLOG(fmt, args...) printk(KERN_ERR fmt"\n", ##args)
        int debug = 1;
#else
#       define DLOG(fmt, args...)
#       define KLOG(fmt, args...)
        int debug = 0;
#endif

// Appelé au chargement du module
static int __init rocketIO_init_module(void)
{
        DLOG("Module loaded");

        return 0;
}

// Appelé au déchargement du module
static void __exit rocketIO_cleanup(void)
{
        DLOG("Module unload");
}

/*****************************************************************************
 * Fonction de l'interface module appelé par le noyau au chargement
 * et déchargement.
 */
module_init(rocketIO_init_module);
module_exit(rocketIO_cleanup);


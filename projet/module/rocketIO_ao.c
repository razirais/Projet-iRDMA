#include <linux/module.h>

#define DEBUG

MODULE_AUTHOR("Geaetan Harter - Adrien Oliva");
MODULE_LICENSE("Dual BSD/GPL");

#ifdef DEBUG
#       define DLOG(fmt, args...) printk("%s: "fmt"\n", __func__, ##args)
#       define KLOG(fmt, args...) printk(KERN_ERR fmt"\n", ##args)
        int debug = 1;
#else
#       define DLOG(fmt, args...)
#       define KLOG(fmt, args...)
        int debug = 0;
#endif

// Appelé au chargement du module
int rocketIO_init_module(void)
{

        return 0;
}

// Appelé au déchargement du module
void rocketIO_cleanup()
{

}






/*****************************************************************************
 * Fonction de l'interface module appelé par le noyau au chargement
 * et déchargement.
 */
int init_module(void)
{
        return rocketIO_init_module();
}

void cleanup_module(void)
{
        rocketIO_cleanup();
}


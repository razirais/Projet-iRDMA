#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>

#include <linux/netdevice.h>

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

/* Constante de fonctionnement du driver */
#define RIO_BASE_ADDR_0         0x000000// adresse matérielle de l'interface
#define RX_BUF_LEN_IDX          2       // taille du buffer de réception
                                        // 0 == 8K, 1 == 16K, 2 == 32K, 3 == 64K
#define TX_BUF_SIZE             1536    // taille du buffer de transmission
#define PKT_BUF_SZ              1536    // (taille max pour TCP, à augmenter pour
                                        //  DDP)
#define TX_TIMEOUT              (6 * HZ)
#define NUM_TX_DESC             4       // nombre de descripteurs de transmission
#define TOTAL_MEM_SIZE_LP       ((8192 << RX_BUF_LEN_IDX) + TX_BUF_SIZE *\
                                NUM_TX_DESC) // taille totale des buffers
#define RIO_RX_INTR             0x0001  // bits de l'ISR
#define RIO_TX_INTR             0x0002

/* Variables globales du driver */
struct net_device       *rio_dev0 = NULL;
struct net_device       *rio_dev1 = NULL;
static char             *hw_addr0;
static char             *hw_addr1;

int                     loopback = 0;

/* Structure privée utilisée par le driver */
struct rio_priv {
        struct  net_device_stats        stats;
        unsigned int                    status;         // utilisé pour simu
        spinlock_t                      lock;
        // Réception
        unsigned char                   *rx_ring;       // adresse du ring buffer
        unsigned int                    cur_rx;         // index réception
        unsigned int                    rx_buf_len;     // taille du ring buffer
        // Transmission
        unsigned int                    cur_tx;         // index d'émission
        unsigned int                    dirty_tx;       // index en cours d'émiss.
        unsigned int                    tx_flag;
        unsigned long                   tx_full;        // file pleine
        // Buffers de transmission
        struct sk_buff                  *tx_skbuff[NUM_TX_DESC];
        unsigned char                   *tx_buf[NUM_TX_DESC];
        unsigned char                   *tx_bufs;
        unsigned int                    tx_packetlen;   // taille du dernier 
                                                        // paquet envoyé
};

static void rocketIO_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
        DLOG("TODO");
}

struct net_device_stats *rocketIO_stats(struct net_device *dev)
{
        DLOG("TODO");
        return NULL;
}

int rocketIO_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
        DLOG("TODO");
        return 0;
}

void d_print_packet(char *pkt, int size, const char *from)
{
        DLOG("TODO");
}

int rocketIO_rx(struct net_device *dev)
{
        DLOG("TODO");
        return 0;
}

int rocketIO_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
        DLOG("TODO");
        return 0;
}

void rocketIO_tx_timeout(struct net_device *dev)
{
        DLOG("TODO");
}

static void rocketIO_init_ring(struct net_device *dev)
{
        DLOG("TODO");
}

int rocketIO_open(struct net_device *dev)
{
        DLOG("TODO");
        return 0;
}

int rocketIO_release(struct net_device *dev)
{
        DLOG("TODO");
        return 0;
}

void rocketIO_init(struct net_device *dev)
{
        struct net_device_ops *operations = 
                kmalloc(sizeof(struct net_device_ops), GFP_KERNEL);
        struct rio_priv *priv = 
                kmalloc(sizeof(struct rio_priv), GFP_KERNEL);

        DLOG("Initialisation of functions used by rocketIO driver");
        
        operations->ndo_init            = rocketIO_init;

        operations->ndo_open            = rocketIO_open;
        operations->ndo_stop            = rocketIO_release;
        operations->ndo_start_xmit      = rocketIO_start_xmit;
        operations->ndo_do_ioctl        = rocketIO_ioctl;
        operations->ndo_get_stats       = rocketIO_stats;
        operations->ndo_tx_timeout      = rocketIO_tx_timeout;

        dev->netdev_ops                 = operations;
        dev->flags                      |= IFF_NOARP | IFF_PROMISC;
        dev->features                   |= NETIF_F_NO_CSUM;
        dev->hard_header_len            = 0;
        // dev->hard_header_cache does not exist anymore !!!
        dev->addr_len                   = 0;
        dev->type                       = 0x1;
        dev->tx_queue_len               = 100;
        dev->mtu                        = 1500;
        
        dev->ml_priv = priv;
        memset(priv, 0, sizeof(struct rio_priv));
        spin_lock_init(&priv->lock);


        DLOG("OK for %s", dev->name);
}

static void rocketIO_cleanup(void);

// Appelé au chargement du module
static int __init rocketIO_init_module(void)
{
        int res, i;
        struct net_device *dev;
        DLOG("Start load module");

        hw_addr0 = kmalloc(7, GFP_KERNEL);
        hw_addr1 = kmalloc(7, GFP_KERNEL);

        memcpy(hw_addr0, "\0ROCK0\0", 7);
        memcpy(hw_addr1, "\0ROCK1\0", 7);

        for (i = 0; i < loopback + 1; i++) {
                dev = alloc_netdev(     sizeof(struct rio_priv),
                                        "rio%d",
                                        rocketIO_init);
                if (i)
                        rio_dev1 = dev;
                else 
                        rio_dev0 = dev;

                if ((res = register_netdev(dev)))
                        KLOG("error %i registering device rio%d", res, i);
                if (!dev) {
                        KLOG("Device not found");
                        rocketIO_cleanup();
                        return -ENODEV;
                }
                DLOG("Device RocketIO found (rio%d)", i);
        }
        if (loopback == 0)
              rio_dev0->base_addr = RIO_BASE_ADDR_0;
        

        

        DLOG("Module loaded");

        return 0;
}

// Appelé au déchargement du module
static void __exit rocketIO_cleanup(void)
{
        if (rio_dev0) {
                DLOG("Unregister rio_dev0");
                unregister_netdev(rio_dev0);
                free_netdev(rio_dev0);
        }
        if (rio_dev1) {
                DLOG("Unregister rio_dev1");
                unregister_netdev(rio_dev1);
                free_netdev(rio_dev1);
        }
        kfree(hw_addr0);
        kfree(hw_addr1);

        DLOG("Module unload");
}

/*****************************************************************************
 * Fonction de l'interface module appelé par le noyau au chargement
 * et déchargement.
 */
module_init(rocketIO_init_module);
module_exit(rocketIO_cleanup);


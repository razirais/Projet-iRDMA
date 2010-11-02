#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>


#include <linux/netdevice.h>


// To use symbolic values of error
#include <linux/errno.h>

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Adrien Oliva, Gaetan Harter");

#define DEBUG


#ifdef DEBUG
#	define DLOG(fmt, args...) printk(KERN_DEBUG "RocketIO %s: "fmt"\n", __func__, ##args)
#	define KLOG(fmt, args...) printk(KERN_ERR "RocketIO :" fmt"\n", ##args)
	int debug = 1;
#else
#	define DLOG(fmt, args...)
#	define KLOG(fmt, args...)
	int debug = 0;
#endif

static int max_interrupt_work = 20;

int loopback = 0;

/* adresses matérielles des interfaces (adresses MAC) */
static char *hw_addr0 = NULL;
static char *hw_addr1 = NULL;

// #####################  A MODIFIER #########################
	/* adresse matérielle de l'interface */
#define RIO_BASE_ADDR_0 0x00000
// ###########################################################


/* taille du buffer de réception */
#define RX_BUF_LEN_IDX	2	/* 0 == 8K, 1 == 16K, 2 == 32K, 3 == 64K */


// #####################  A MODIFIER #########################
	/* à augmenter pour DDP */
/* taille des buffers de transmission (taille maximale d'un paquet TCP). */
#define TX_BUF_SIZE	1536
#define PKT_BUF_SZ	1536
// ###########################################################

#define TX_TIMEOUT  (6 * HZ)

/* Nombre de descripteurs de transmission */
#define NUM_TX_DESC	4

/* taille totale des buffers */
#define TOTAL_MEM_SIZE_LP	((8192 << RX_BUF_LEN_IDX) + TX_BUF_SIZE * NUM_TX_DESC)


// #####################  A MODIFIER #########################
/* bits de l'ISR */
#define RIO_RX_INTR 0x0001
#define RIO_TX_INTR 0x0002
// ###########################################################

void rocketIO_init(struct net_device *dev);

struct net_device *rio_dev0 = NULL;
struct net_device *rio_dev1 = NULL;

/* structure privée utilisée par le driver */
struct rio_priv {
	struct net_device_stats stats;

	/* utilisé pour le mode simulé
	   équivaut à l'ISR matériel */
	unsigned int status;

	/* spinlock de la structure */
	spinlock_t lock;

	/* Réception. */
	unsigned char *rx_ring;	/* adresse du ring buffer */
	unsigned int cur_rx;	/* Index ring buffer du prochain paquet reçu */
	unsigned int rx_buf_len;/* taille du buffer ring */

	/* Transmission */
	unsigned int cur_tx; 	/* descripteur à utiliser pour une transmission */
	unsigned int dirty_tx; 	/* 1er descripteur qui n'a pas fini de transmettre */
	unsigned int tx_flag;
	unsigned long tx_full; 	/* file de transmission pleine */

	/* buffers utilisés pour la transmission */
	struct sk_buff 	*tx_skbuff[NUM_TX_DESC];
	unsigned char 	*tx_buf[NUM_TX_DESC];
	unsigned char 	*tx_bufs;
	/* taille du dernier paquet envoyé */
	unsigned int tx_packetlen;
};



/* routine de traitement des interruptions */
static void rocketIO_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	DLOG("DUMMY");
}

/*
 * imprime les statistiques (utilisé par ifconfig)
 */
struct net_device_stats *rocketIO_stats(struct net_device *dev)
{
	/*struct rio_priv *priv = dev->priv;
	return &(priv->stats);
	*/
	DLOG("DUMMY");
	return NULL;
}

/*
 * Ioctl commands
 */
int rocketIO_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	DLOG("DUMMY");
	return 0;
}

/* imprime un paquet (pour debug) */
void d_print_packet(char *pkt, int size, const char *from)
{
#ifdef DEBUG
	int i = 0;
	int *tmp = (int *) pkt;
	printk("d_print_packet: %s\n", from);
	while (i < size && (size - i) >= 4) {
		printk("%x\n", *tmp);
		tmp += 1;
		i += 4;
	}
#else
	(void)pkt;
	(void)size;
	(void)from;
#endif
}

/* traite une interruption de réception */
int rocketIO_rx(struct net_device *dev)
{
	DLOG("DUMMY");
	return 0;
}


/* fonction de transmission */
int rocketIO_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	DLOG("DUMMY");
	return 0;
}
/*
 * traite un transmit timeout.
 */
void rocketIO_tx_timeout(struct net_device *dev)
{
	DLOG("DUMMY");
	return;
}


/* initialisatin des buffers. */
static void rocketIO_init_ring(struct net_device *dev)
{
	DLOG("DUMMY");
	return;
}


int rocketIO_open(struct net_device *dev)
{
	DLOG("DUMMY");
	return 0;
}

int rocketIO_release(struct net_device *dev)
{
	DLOG("DUMMY");
	return 0;
}

void rocketIO_init(struct net_device *dev)
{
	DLOG("DUMMY");

	ether_setup(dev);
	return;
}


int rocketIO_init_module(void)
{
	DLOG("Starting module");
	return 0;
}
void rocketIO_cleanup(void)
{
	if (rio_dev0) {
		unregister_netdev(rio_dev0);
		free_netdev(rio_dev0);
	} 
	if (rio_dev1) {
		unregister_netdev(rio_dev1);
		free_netdev(rio_dev1);
	}
	DLOG("OK");
	return;
	DLOG("Exiting module");
}


static int __init initialization_function(void)
{
	return rocketIO_init_module();
}
static void __exit cleanup_function(void)
{
	rocketIO_cleanup();
}
module_init(initialization_function);
module_exit(cleanup_function);


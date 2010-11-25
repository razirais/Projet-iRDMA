#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>


#include <linux/netdevice.h>

#include "rocketIO.h"


// To use symbolic values of error
#include <linux/errno.h>

/* adresses matérielles des interfaces (adresses MAC) */
static char *hw_addr0 = NULL;
static char *hw_addr1 = NULL;

struct net_device *rio_dev0 = NULL;
struct net_device *rio_dev1 = NULL;
int dev_is_registered[2] = {0};



/* structure privée utilisée par le driver */
struct rio_priv {
#ifdef SIMULE
	unsigned int status; 	/* mode simulé: équivaut à l'ISR matériel */
#endif
	struct net_device_stats stats;

	spinlock_t lock;	/* spinlock de la structure */

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
	unsigned int 	tx_packetlen; /* taille du dernier paquet envoyé */
};



/* routine de traitement des interruptions */
//static 
void rocketIO_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	DLOG("DUMMY");
}

/*
 * imprime les statistiques (utilisé par ifconfig)
 */
// DONE
struct net_device_stats *op_rio_stats(struct net_device *dev)
{
	struct rio_priv *priv = netdev_priv(dev);
	DLOG();
	return &(priv->stats);
}

/*
 * Ioctl commands
 */
int op_rio_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
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
int op_rio_rx(struct net_device *dev)
{
	DLOG("DUMMY");
	return 0;
}


/* fonction de transmission */
int op_rio_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	DLOG("DUMMY");
	return 0;
}
/*
 * traite un transmit timeout.
 */
void op_rio_tx_timeout(struct net_device *dev)
{
	DLOG("DUMMY");
	return;
}


/* initialisatin des buffers. */
//static 
void IN_init_ring(struct net_device *dev)
{
	struct rio_priv *priv = netdev_priv(dev);
	int i;

	priv->tx_full = 0;
	priv->dirty_tx = priv->cur_tx = 0;

	for (i = 0; i < NUM_TX_DESC; i++) {
		priv->tx_skbuff[i] = 0;
	}
	DLOG("tx_skbuff=%x tx_bufs=%x", 
		(unsigned int) priv->tx_skbuff[0], (unsigned int) priv->tx_bufs);
	DLOG("for %s OK", dev->name);
	return;
}

/*
 *
 * On alloue un buffer de taille comprise entre 
 *  RX_BUF_LEN_IDX
 * // 0 == 8K, 1 == 16K, 2 == 32K, 3 == 64K 
 *
 * et 8k
 *
 */
int IN_alloc_ring(struct net_device *dev)
{
	struct rio_priv *priv = netdev_priv(dev);
	int rx_buf_len_idx = RX_BUF_LEN_IDX;

	/* 
	 * le ring sera place à 7f0 000 
	 * il faut récupérer la valeur dans le 
	 * dans device tree champ buffer base
	 * 
	 */ 


	do {
		priv->rx_buf_len = 8192 << rx_buf_len_idx;
		priv->rx_ring = kmalloc(priv->rx_buf_len + 16 +
					(TX_BUF_SIZE * NUM_TX_DESC),
					GFP_KERNEL);
		priv->cur_rx = (unsigned int) priv->rx_ring;
	} while (priv->rx_ring == NULL && --rx_buf_len_idx >= 0);
	

	if (priv->rx_ring == NULL) {
		KLOG("%s: Couldn't allocate a %d byte receive ring.",
			       dev->name, priv->rx_buf_len);
		return -ENOMEM;
	}
	DLOG("rx_ring: %x, cur_rx: %x, size: %uK", 
			(unsigned int) priv->rx_ring, 
			(unsigned int) priv->cur_rx, 
			priv->rx_buf_len >> 10);

	priv->tx_bufs = priv->rx_ring + priv->rx_buf_len + 16;

	return 0;
}


int op_rio_open(struct net_device *dev)
{
	/* alloue le chanel d'interruption */
	/*
	if (request_irq(dev->irq, 
			&rocketIO_interrupt, 
			SA_SHIRQ, dev->name, dev)) {
		return -EAGAIN;
	}
	*/

	IN_alloc_ring(dev);
	IN_init_ring(dev);


	/* adresse MAC des interfaces */
	if (dev == rio_dev0)
		memcpy(dev->dev_addr, hw_addr0, 6);
	else
		memcpy(dev->dev_addr, hw_addr1, 6);

	/* démarrage de l'interface */
	netif_start_queue(dev);

	DLOG("for %s OK", dev->name);
	return 0;
}

int op_rio_release(struct net_device *dev)
{
	struct rio_priv *priv = netdev_priv(dev);
	DLOG("%s", dev->name);
	/* release ports, irq and such -- like fops->close */

	if (priv->rx_ring) {
		DLOG("%s kfree ring", dev->name);
		kfree(priv->rx_ring);
	}

	netif_stop_queue(dev);
	return 0;
}

int op_rio_reg_init(struct net_device *dev) 
{
	DLOG("%s", dev->name);
	return 0;
}
void op_rio_reg_uninit(struct net_device *dev) 
{
	DLOG("%s", dev->name);
	return;
}

/*
 *
 * Structure contenant les adresses des opérations sur le driver
 *
 */
struct net_device_ops rocketIO_ops = {
	.ndo_init       = &op_rio_reg_init,
	.ndo_uninit     = &op_rio_reg_uninit,
        .ndo_open       = &op_rio_open,
        .ndo_stop       = &op_rio_release,
        .ndo_start_xmit = &op_rio_start_xmit,
        .ndo_do_ioctl   = &op_rio_ioctl,
        .ndo_get_stats  = &op_rio_stats,
        .ndo_tx_timeout = &op_rio_tx_timeout
	/*
	 * dev->open = rocketIO_open;
	 * dev->stop = rocketIO_release;
	 * dev->hard_start_xmit = rocketIO_start_xmit;
	 * dev->do_ioctl = rocketIO_ioctl;
	 * dev->get_stats = rocketIO_stats;
	 * dev->tx_timeout = rocketIO_tx_timeout;
	 * //dev->set_config = rocketIO_config;
	 * //dev->change_mtu = rocketIO_change_mtu;
	 * //dev->rebuild_header = rocketIO_rebuild_header;
	 * //dev->hard_header = rocketIO_header;
	 * //dev->watchdog_timeo = timeout;
	 */
};

void IN_rocketIO_init(struct net_device *dev)
{
	struct rio_priv *priv;
	DLOG("DUMMY");
	

	ether_setup(dev);
	/* dev->header_ops	= &eth_header_ops;
	 * dev->type		= ARPHRD_ETHER;
	 * dev->hard_header_len	= ETH_HLEN;
	 * dev->mtu		= ETH_DATA_LEN;
	 * dev->addr_len	= ETH_ALEN;
	 * dev->tx_queue_len	= 1000;	// Ethernet wants good queues 
	 * dev->flags		= IFF_BROADCAST|IFF_MULTICAST;
	 * memset(dev->broadcast, 0xFF, ETH_ALEN);
	 */
	dev->netdev_ops = &rocketIO_ops;
	

	/*

	dev->flags |= IFF_NOARP | IFF_PROMISC;
	dev->features | = NETIF_F_NO_CSUM;
	dev->hard_header_cache = NULL;//
	dev->addr_len = 0;
	dev->hard_header_len = 0;
	dev->type = 0x1;
	dev->tx_queue_len = 100;
	dev->mtu = 1500;

	SET_MODULE_OWNER(dev);
	*/

	/*
	 * Then, initialize the priv field. This encloses the statistics
	 * and a few private fields.
	 */
	priv = netdev_priv(dev);
	memset(priv, 0, sizeof(struct rio_priv));
	spin_lock_init(&priv->lock);
	// st


	DLOG("for %s OK", dev->name);
	return;
}

/* 
 * adresses MAC (7 octets) 
 * Elles commencent par \0 pour ne pas avoir d'adresse de type multicast
 * Si la valeur retournée est négative, il faudra faire un appel à cleanup
 */
int IN_init_addr(void)
{
	DLOG();
	hw_addr0 = kmalloc(7, GFP_KERNEL);
	if (!hw_addr0)
		goto error;
	hw_addr1 = kmalloc(7, GFP_KERNEL);
	if (!hw_addr1)
		goto error;

	memcpy(hw_addr0, "\0ROCK0\0", 7);
	memcpy(hw_addr1, "\0ROCK1\0", 7);
	return 0;
error:                                  // a revoir si clean de mémoire
	return -EAGAIN;
}

/*
 * Alloue un netdev, et l'enregistre.
 * met à jour la variable d'etat dev_is_registered[num]
 * 
 * num = 0 ou 1 == numéro de l'interface
 */

// va faire planter tant que IN_rocketIO_init est pas finit
int IN_register_netdev(int num)
{
	int res;
	struct net_device *dev;
	DLOG();

	if (num != 0 && num != 1) 
		return -EINVAL;


	/* %d est auto incremente par alloc_netdev */
	dev = alloc_netdev(sizeof(struct rio_priv), "rio%d", IN_rocketIO_init);

	if (num == 0)
		rio_dev0 = dev;
	else
		rio_dev1 = dev;

	if ((res = register_netdev(dev))) {
		KLOG("error %i registering device rio%d", res, num);
		dev_is_registered[num] = 0;
		return -EAGAIN;
	} else {
		dev_is_registered[num] = 1;
	}
		

	if (!dev) {
		DLOG("Device not found");
		return -ENODEV;
	}

	DLOG("Device found : rio%d", num);

	return 0;
}

void rocketIO_cleanup(void)
{
	DLOG();
	if (rio_dev0) {
		if (dev_is_registered[0]) {
			unregister_netdev(rio_dev0);
			DLOG("Unregistering netdev rio_dev%u", 0);
		}
		free_netdev(rio_dev0);
	} 
	if (rio_dev1) {
		if (dev_is_registered[1]) {
			unregister_netdev(rio_dev1);
			DLOG("Unregistering netdev rio_dev%u", 1);
		}
		free_netdev(rio_dev1);
	}
	if (hw_addr0)
		kfree(hw_addr0);
	if (hw_addr1)
		kfree(hw_addr1);

	DLOG("OK");
	return;
	DLOG("Exiting module");
}
int rocketIO_init_module(void)
{
	int i;
	DLOG("Starting module");


	if (IN_init_addr() < 0)
		goto error;

	for (i = 0; i < loopback + 1; i++) 
		if (IN_register_netdev(i) < 0) 
			goto error;
	
	if (loopback == 0)
		rio_dev0->base_addr = RIO_BASE_ADDR_0;

	return 0;
error:
	rocketIO_cleanup();
	return -EAGAIN;
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


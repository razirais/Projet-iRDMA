#define MODULE
#define __KERNEL__

#include <linux/module.h>
#include <linux/config.h>
#include <linux/skbuff.h>

#include <linux/netdevice.h>

//#include <netinet/in.h>
//#include <arpa/inet.h>
#include <linux/in.h>
#include <linux/ip.h>		/* struct iphdr */
#include <linux/tcp.h>


MODULE_AUTHOR("Vincent Degironde");
MODULE_LICENSE("Dual BSD/GPL");

MODULE_PARM(debug, "i");
MODULE_PARM_DESC(debug, "Driver message level (0-1)");

MODULE_PARM(loopback, "i");
MODULE_PARM_DESC(loopback, "Loopback interface for debugging purposes");



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


static int max_interrupt_work = 20;

int loopback = 0;

/* adresses matérielles des interfaces (adresses MAC) */
static char *hw_addr0;
static char *hw_addr1;

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

struct net_device *rio_dev0;
struct net_device *rio_dev1;

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
	int statusword; /* registre d'interruptions */
	int entry;
	struct rio_priv *priv;

	struct net_device *dev = (struct net_device *) dev_id;

	if (!dev)
		return;

	DLOG();

	priv = (struct rio_priv *) dev->priv;

	spin_lock(&priv->lock);

	if (loopback == 1) {
		statusword = priv->status;
	} else {
		// #####################  A MODIFIER #########################
		/* lire ISR et l'affecter à statusword */
		// ###########################################################
		printk("rocketIo_interrupt : A modifier\n");
	}
	if (statusword & RIO_RX_INTR) {	/* interruption de réception */
		DLOG("RX_INT.rx_buf_len= %d cur_rx= %x",
			priv->rx_buf_len, priv->cur_rx);
		rocketIO_rx(dev);
	}
	if (statusword & RIO_TX_INTR) {	/* interruption de transmission */
		DLOG("TX_INT.cur_tx= %x dirty_rx= %d",
			priv->cur_tx, priv->dirty_tx);
		entry = priv->dirty_tx;
		/* on parcourt la liste des descripteurs jusqu'à tomber sur un qui n'a pas finin d'envoyer */
		while (priv->cur_tx - entry > 0) {  /* Gaetan : j'ai pas compris la condition */
			/* mise à jour des stats */
			priv->stats.tx_packets++;
			priv->stats.tx_bytes += priv->tx_packetlen;

			/* libération du socket buffer */
			dev_kfree_skb(priv->tx_skbuff[entry]);
			priv->tx_skbuff[entry] = 0;

			/* si la file était pleine, on la relance */
			if (test_bit(0, &priv->tx_full)) {
				/* The ring is no longer full, clear tbusy. */
				clear_bit(0, &priv->tx_full);
				netif_resume_tx_queue(dev);
			}
			entry++;
		}
		priv->dirty_tx = entry;
	}

	spin_unlock(&priv->lock);
	return;
}


/*
 * imprime les statistiques (utilisé par ifconfig)
 */
struct net_device_stats *rocketIO_stats(struct net_device *dev)
{
	struct rio_priv *priv = dev->priv;
	return &(priv->stats);
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
	DLOG("dev = 0x%x", dev);
	struct sk_buff *skb;
	struct rio_priv *priv = (struct rio_priv *) dev->priv;
	int len, res;
	int cur_rx = priv->cur_rx;
	unsigned char *rx_ring = priv->rx_ring;
	int ring_offset = cur_rx % priv->rx_buf_len;
	int *tmp;
	u32 rx_status = 0;

	/* on récupère l'adresse du paquet dans le ring buffer et sa taille */
	if (loopback == 1) {
		tmp = ((int *) (rx_ring + ring_offset));
		len = (int) (*((char *) tmp + 3));
	} else {
		// #####################  A MODIFIER #########################
		/* lire rx_status à l'adresse rx_ring + rx_offset */
		// ###########################################################
		/* rx_status = 4 premiers octets i.e valeur du registre copié en en-tête */
		len = rx_status >> 16;
		tmp = ((int *) (rx_ring + ring_offset + 4));
	}
	/* création d'un socket buffer pour la couche supérieure */
	skb = dev_alloc_skb(len + 2);
	if (!skb) {
		printk("rocketIO_rx: low on mem - packet dropped\n");
		priv->stats.rx_dropped++;
		return;
	}
	skb_reserve(skb, 2);	/* align IP on 16B boundary */
	/* copie du paquet dans le socket buffer */
	memcpy(skb_put(skb, len), (void *) tmp, len);

	/* on affecte qqs champs du skb */
	skb->dev = dev;
	if (dev == rio_dev0)
		skb->mac.raw = hw_addr0;
	else
		skb->mac.raw = hw_addr1;

	skb->protocol = 0x0800; /* IP */
	skb->pkt_type = PACKET_HOST;


	skb->ip_summed = CHECKSUM_UNNECESSARY;	/* pas la peine */

	/* mise à jour des stats */
	priv->stats.rx_packets++;
	priv->stats.rx_bytes += len;

	d_print_packet(skb->data, len, "rocketIO_rx");

	/* passe le skb à la couche supérieure */
	res = netif_rx(skb);

	/* mise à jour de cur_rx */
	if (loopback == 1) {
		priv->cur_rx = (cur_rx + len);
	} else {
		priv->cur_rx = (cur_rx + len + 4);
		// #####################  A MODIFIER #########################
		/* mettre à jour du registre CAPR */
		// ###########################################################
	}
	return 0;
}

/* fonction de transmission */
int rocketIO_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	int *tmp;
	char *tmp2;

	struct iphdr *ih;
	struct net_device *dest;
	struct rio_priv *priv = (struct rio_priv *) dev->priv;
	u32 *saddr;
	u32 *daddr;
	int len = skb->len;
	int entry = priv->cur_tx % NUM_TX_DESC;

	priv->tx_skbuff[entry] = skb;

	DLOG("len = %d dev = 0x%x", len, dev);
	d_print_packet(skb->data, len, "rocketIO_tx");

	/* I am paranoid. Ain't I? */
	if (len < sizeof(struct iphdr)) {
		printk("rocketIO: Hmm... packet too short (%i octets)\n", len);
		return -1;
	}

	netif_pause_tx_queue(dev);

	if (loopback == 1) {	/* mode simulé */
		/* changement des adresses IP (cf rapport simulation de l'interface */
		ih = (struct iphdr *) ((char *) skb->data);
		saddr = &ih->saddr;
		daddr = &ih->daddr;
		((u8 *) saddr)[2] ^= 1;	/* change the third octet (class C) */
		((u8 *) daddr)[2] ^= 1;
		ih->check = 0;	/* and rebuild the checksum (ip needs it) */
		ih->check = ip_fast_csum((unsigned char *) ih, ih->ihl);

		DLOG("saddr = 0x%x daddr = 0x%x", *saddr, *daddr);

		/* qui envoie ? qui reçoit ? */
		if (dev == rio_dev0)
			dest = rio_dev1;
		else
			dest = rio_dev0;


		DLOG("rio_dev0 = 0x%x dev= 0x%x dest= 0x%x sizeof(dev)= %d",
			     (void *) rio_dev0, (void *) dev,
			     (void *) dest, sizeof(struct net_device));

		/* côté réception */
		priv = (struct rio_priv *) dest->priv;
		/* interruption de réception */
		priv->status = RIO_RX_INTR;

		/* copie du paquet "à la main" en buffer de réception */
		tmp2 =
		    (char *) (priv->rx_ring +
			      (priv->cur_rx % priv->rx_buf_len));
		memcpy(tmp2, skb->data, len);
		/* "déclenchement" de l'interruption */
		rocketIO_interrupt(0, dest, NULL);

		/* côté transmission */
		priv = (struct rio_priv *) dev->priv;
		/* interrutpion de transmission */
		priv->status = RIO_TX_INTR;
		/* mise à jour des champs adéquats */
		priv->tx_packetlen = len;
		priv->cur_tx = (priv->cur_tx + 1);
		priv->dirty_tx = entry;

		/* jiffies = temps actuel (unité proportionnelle à la fréquence du processeur */
		dev->trans_start = jiffies;
		/* déclenchement de l'interruption */
		rocketIO_interrupt(0, dev, NULL);
	} else {		/* cas réel */
		priv->tx_packetlen = len;
		priv->cur_tx = (priv->cur_tx + 1);
		dev->trans_start = jiffies;

		/* copie du skb dans le buffer de transmission */
		priv->tx_skbuff[entry] = skb;

		// #####################  A MODIFIER #########################
		/* écriture des registres TSAD[entry] (&priv->tx_skbuff[entry]) et TSD[entry] */
		// ###########################################################

		/* vérif : est-ce que la file de transmission est pleine ? i.e les 4 descipteurs utilisés */
		if (priv->cur_tx - priv->dirty_tx >= NUM_TX_DESC) {
			set_bit(0, &priv->tx_full);
			/** Gaetan: on est pas en session critique là, il se passe quoi si ça merde ? */
			/* vérification si une interruption est intervenue entre temps */
			if (priv->cur_tx -
			    (volatile unsigned int) priv->dirty_tx <
			    NUM_TX_DESC) {
				clear_bit(0, &priv->tx_full);
				netif_unpause_tx_queue(dev);
			} else {
				netif_stop_tx_queue(dev);
			}
		} else {
			netif_unpause_tx_queue(dev);
		}

		dev->trans_start = jiffies;
	}
	return 0;
}

/*
 * traite un transmit timeout.
 */
void rocketIO_tx_timeout(struct net_device *dev)
{
	DLOG("DUMMY");
}


/* initialisatin des buffers. */
static void rocketIO_init_ring(struct net_device *dev)
{
	struct rio_priv *priv = dev->priv;
	int i;

	priv->tx_full = 0;
	priv->dirty_tx = priv->cur_tx = 0;

	for (i = 0; i < NUM_TX_DESC; i++) {
		priv->tx_skbuff[i] = 0;
	}
	DLOG("tx_skbuff=%x tx_bufs=%x", 
		priv->tx_skbuff[0], priv->tx_bufs);
	DLOG("for %s OK", dev->name);
}


int rocketIO_open(struct net_device *dev)
{
	struct rio_priv *priv = dev->priv;
	long ioaddr = dev->base_addr;
	int rx_buf_len_idx;

	MOD_INC_USE_COUNT;

	/* alloue le chanel d'interruption */
	if (request_irq(dev->irq, 
			&rocketIO_interrupt, 
			SA_SHIRQ, dev->name, dev)) {
		MOD_DEC_USE_COUNT;
		return -EAGAIN;
	}

	/* allocation des buffers */
	rx_buf_len_idx = RX_BUF_LEN_IDX;
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
		MOD_DEC_USE_COUNT;
		return -ENOMEM;
	}
	/* Gaetan : on devrait afficher la taille du buffer alloué vu que c'est pas constant */
	DLOG("rx_ring = %x cur_rx = %x", priv->rx_ring, priv->cur_rx);

	priv->tx_bufs = priv->rx_ring + priv->rx_buf_len + 16;
	rocketIO_init_ring(dev);

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

/* appélé lorsque qu'on éteint les interfaces */
int rocketIO_release(struct net_device *dev)
{
	/* release ports, irq and such -- like fops->close */

	netif_stop_queue(dev);	/* can't transmit any more */
	MOD_DEC_USE_COUNT;
	/* if irq2dev_map was used (2.0 kernel), zero the entry here */
	DLOG("for %s OK", dev->name);
	return 0;
}

/* initialisation des structures netdevice et rio_priv */
void rocketIO_init(struct net_device *dev)
{
	struct rio_priv *priv;

	dev->open = rocketIO_open;
	dev->stop = rocketIO_release;
	dev->hard_start_xmit = rocketIO_start_xmit;
	dev->do_ioctl = rocketIO_ioctl;
	dev->get_stats = rocketIO_stats;
	dev->tx_timeout = rocketIO_tx_timeout;
	//dev->set_config = rocketIO_config;
	//dev->change_mtu = rocketIO_change_mtu;
	//dev->rebuild_header = rocketIO_rebuild_header;
	//dev->hard_header = rocketIO_header;
	//dev->watchdog_timeo = timeout;

	dev->flags |= IFF_NOARP | IFF_PROMISC;
	dev->features | = NETIF_F_NO_CSUM;
	dev->hard_header_cache = NULL;	/* Disable caching */
	dev->addr_len = 0;
	dev->hard_header_len = 0;
	dev->type = 0x1;
	dev->tx_queue_len = 100;
	dev->mtu = 1500;

	SET_MODULE_OWNER(dev);
	/*
	 * Then, initialize the priv field. This encloses the statistics
	 * and a few private fields.
	 */
	priv = dev->priv;
	memset(priv, 0, sizeof(struct rio_priv));
	spin_lock_init(&priv->lock);
	DLOG("for %s OK", dev->name);
}


/* appelé au déchargement du module */
void rocketIO_cleanup()
{
	int i;
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
}

/* appelé au chargement du module */
int rocketIO_init_module(void)
{
	int res, i;
	struct net_device *dev;

	hw_addr0 = kmalloc(7, GFP_KERNEL);
	hw_addr1 = kmalloc(7, GFP_KERNEL);

	/* adresses MAC (7 octets) (le premier \0 est nécessaire pour être sur de ne pas avoir une adresse de type multicast */
	memcpy(hw_addr0, "\0ROCK0\0", 7);
	memcpy(hw_addr1, "\0ROCK1\0", 7);

	for (i = 0; i < loopback + 1; i++) {
		dev = alloc_netdev(sizeof(struct rio_priv), 
				"rio%d",
				rocketIO_init);
		if (i == 0)
			rio_dev0 = dev;
		else
			rio_dev1 = dev;

		if (res = register_netdev(dev))
			printk(KERN_ERR
			       "rocketIO : error %i registering device rio%d",
			       res, i);
		if (!dev) {
			printk("Device RocketIO not found\n");
			rocketIO_cleanup();
			return -ENODEV;
		}

		printk("Device RocketIO found : rio%d\n", i);
	}
	if (loopback == 0)
		rio_devs[0].base_addr = RIO_BASE_ADDR_0;
	return 0;
}


/* fonctions nécessitées par le noyau pour un module */
int init_module(void)
{
	return rocketIO_init_module();
}

void cleanup_module(void)
{
	rocketIO_cleanup();
}


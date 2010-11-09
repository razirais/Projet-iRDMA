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
	unsigned int status; /* mode simulé: équivaut à l'ISR matériel */
#endif
	struct net_device_stats stats;

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
	unsigned int tx_packetlen; /* taille du dernier paquet envoyé */
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

/* 
 * adresses MAC (7 octets) 
 * Elles commencent par \0 pour ne pas avoir d'adresse de type multicast
 * Si la valeur retournée est négative, il faudra faire un appel à cleanup
 */
int IN_init_addr(void)
{
	hw_addr0 = kmalloc(7, GFP_KERNEL);
	if (!hw_addr0)
		goto error;
	hw_addr1 = kmalloc(7, GFP_KERNEL);
	if (!hw_addr1)
		goto error;

	memcpy(hw_addr0, "\0ROCK0\0", 7);
	memcpy(hw_addr1, "\0ROCK1\0", 7);
	return 0;
error:
	return -EAGAIN;
}

/*
 * num = 0 ou 1 == numéro de l'interface
 */

// va faire planter tant que rocketIO_init est pas finit

int IN_register_netdev(int num)
{
	int res;
	struct net_device *dev;

	if (num != 0 && num != 1) 
		return -EINVAL;

	/* %d est auto incremente par alloc_netdev */
	dev = alloc_netdev(sizeof(struct rio_priv), "rio%d", rocketIO_init);
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
		DLOG("Device not found\n");
		return -ENODEV;
	}

	DLOG("Device found : rio%d\n", num);

	return 0;
}

void rocketIO_cleanup(void)
{
	if (rio_dev0) {
		if (dev_is_registered[0])
			unregister_netdev(rio_dev0);
		free_netdev(rio_dev0);
	} 
	if (rio_dev1) {
		if (dev_is_registered[1])
			unregister_netdev(rio_dev1);
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


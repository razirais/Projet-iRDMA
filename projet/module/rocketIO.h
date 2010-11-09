
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


#define SIMULE

#ifdef SIMULE
int loopback = 1;
#else
int loopback = 0;
#endif


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

#define NUM_TX_DESC	4 	/* Nombre de descripteurs de transmission */

/* taille totale des buffers */
#define TOTAL_MEM_SIZE_LP	((8192 << RX_BUF_LEN_IDX) + TX_BUF_SIZE * NUM_TX_DESC)


// #####################  A MODIFIER #########################
/* bits de l'ISR */
#define RIO_RX_INTR 0x0001
#define RIO_TX_INTR 0x0002
// ###########################################################


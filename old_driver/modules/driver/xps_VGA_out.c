

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/xilinx_devices.h>
#include <asm/uaccess.h>

#include <asm/delay.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/signal.h>

#include <linux/cdev.h>

#include <linux/dma-mapping.h>
#include <linux/dmapool.h>


#if defined(CONFIG_OF)
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_i2c.h>
#endif


typedef volatile struct {
	volatile unsigned int ISR;
	volatile unsigned int IPR;
	volatile unsigned int IER;
	volatile unsigned int toto[3];
	volatile unsigned int IDR;
	volatile unsigned int GIER;
} Xps_VGA_out_Interrupt_IPIF;
typedef volatile struct {
	volatile unsigned int ISR;
	volatile unsigned int ISR_b;
	volatile unsigned int IER;
} Xps_VGA_out_Interrupt;

typedef struct Xps_VGA_out_Control {
	volatile unsigned int start;
	volatile unsigned int addr_image;
	volatile unsigned int auto_start;
	volatile unsigned int it_enable;
	volatile unsigned int it;
} Xps_VGA_out_Control;

struct Xps_VGA_out_device {
	volatile void *hw_phy_base;
	volatile int hw_length;
	volatile void *hw_base;

	volatile Xps_VGA_out_Interrupt_IPIF *it_ipif;
	volatile Xps_VGA_out_Interrupt *it;
	volatile Xps_VGA_out_Control *control;

	wait_queue_head_t wait;

	int irq;
	struct device *dev;
	int id;
	struct cdev cdev;
	struct resource *iomem_res;
	int irq_pass;
};

static int vga_out_major = 0;
static int vga_out_minor = 0;
static int vga_out_nr_devs = 1;
#define SYNCHRONIZE_IO __asm__ volatile ("eieio")	/* should be 'mbar' ultimately */


MODULE_LICENSE("GPL");


/* --------- *
 * Interrupt *
 * --------- */

static irqreturn_t xps_VGA_out_isr(int irq, void *dev_id)
{
	struct Xps_VGA_out_device *vga_out = dev_id;
	printk("I");
	/* A COMPLETER */
	return IRQ_HANDLED;
}

static void xps_VGA_out_IRQ_reset(struct Xps_VGA_out_device *vga_out)
{
	vga_out->it_ipif->IER = 0;
	vga_out->it_ipif->GIER = 0;
	vga_out->it->IER = 0;
	vga_out->it_ipif->ISR = 0;
	vga_out->it->ISR = 0;
}

static void xps_VGA_out_IRQ_init(struct Xps_VGA_out_device *vga_out)
{
	vga_out->it_ipif->IER = 4;
	vga_out->it_ipif->GIER = 0xFFFFFFFF;
	vga_out->it->IER = 0xFFFFFFFF;
	vga_out->it_ipif->ISR = 0;
	vga_out->it->ISR = 0;
	vga_out->control->it_enable = 1;
}

/* ---------------- *
 * Character driver *
 * ---------------- */


static ssize_t xps_VGA_out_read(struct file *filep,
				char *buf, size_t count, loff_t * offset)
{
	struct Xps_VGA_out_device *vga_out = filep->private_data;
	printk("R");
	/* A COMPLETER */
	*buf = 0;
	*offset += count;
	return count;
}


static ssize_t xps_VGA_out_write(struct file *filep,
				 const char *buf,
				 size_t count, loff_t * offset)
{
	struct Xps_VGA_out_device *vga_out = filep->private_data;
	unsigned int addr;
	/* A COMPLETER */
	(*offset) += count;
	return count;
}



static int xps_VGA_out_open(struct inode *inode, struct file *filep)
{
	struct Xps_VGA_out_device *vga_out;
	printk("vga_out: open\n");
	vga_out =
	    container_of(inode->i_cdev, struct Xps_VGA_out_device, cdev);
	filep->private_data = vga_out;
	nonseekable_open(inode, filep);
	printk("vga_out: open OK\n");
	vga_out->irq_pass = 1;
	return 0;
}

static int xps_VGA_out_release(struct inode *inode, struct file *filep)
{
	struct Xps_VGA_out_device *vga_out = filep->private_data;
	xps_VGA_out_IRQ_reset(vga_out);
	vga_out->control->it_enable = 0;

	return 0;
}

static struct file_operations vga_out_fops = {
	.read = xps_VGA_out_read,
	.write = xps_VGA_out_write,
	.open = xps_VGA_out_open,
	.release = xps_VGA_out_release,
};

/* ------ *
 * Module *
 * ------ */


/* Le reste de ce code n'est pas le plus important */

static int vga_out_setup(struct Xps_VGA_out_device *vga_out)
{
	int ret;
	int port;

	init_waitqueue_head(&vga_out->wait);

	port = check_mem_region(vga_out->hw_phy_base, vga_out->hw_length);
	if (port) {
		printk("<1>vga_out: cannot reserve 0x%x!\n",
		       vga_out->hw_phy_base);
		return port;
	} else {
		vga_out->iomem_res =
		    request_mem_region(vga_out->hw_phy_base,
				       vga_out->hw_length, "vga_out");
		printk("<1>vga_out: 0x%x (hw_phy) reserved!\n",
		       vga_out->hw_phy_base);
	}
	vga_out->hw_base =
	    ioremap(vga_out->hw_phy_base, vga_out->hw_length);
	vga_out->it_ipif = (vga_out->hw_base + 0x300);
	vga_out->it = (vga_out->hw_base + 0x300 + 0x20);
	vga_out->control = (vga_out->hw_base + 0x0);

	printk
	    ("<1>vga_out: registers remapped at 0x%x (virt)\nIPIF : 0x%x\nIT : 0x%x\nCtrl 0x%x\n",
	     vga_out->hw_base, vga_out->it_ipif, vga_out->it,
	     vga_out->control);


	ret = request_irq(vga_out->irq,
			  xps_VGA_out_isr,
			  IRQF_TRIGGER_RISING | IRQF_DISABLED,
			  "vga_out_isr", vga_out);
	if (ret) {
		printk("<1>vga_out: failed setting IRQ\n");
	} else {
		xps_VGA_out_IRQ_init(vga_out);
		printk("<1>vga_out: ISR  has been set\n");
	}

	printk("<1>vga_out: loaded successfully\n");
	return 0;
}


static void __devexit vga_out_teardown(struct Xps_VGA_out_device *vga_out)
{
	printk("vga_out: teardown...");
	xps_VGA_out_IRQ_reset(vga_out);
	vga_out->control->it_enable = 0;
	cdev_del(&vga_out->cdev);
	iounmap(vga_out->hw_base);
	release_mem_region((unsigned int) vga_out->hw_phy_base,
			   vga_out->hw_length);
	free_irq(vga_out->irq, vga_out);
	printk("teardown OK\n");
}

static void vga_out_setup_cdev(struct Xps_VGA_out_device *vga_out,
			       int index)
{
	int err, devno = MKDEV(vga_out_major, vga_out_minor + index);
	printk("vga_out_setup_cdev\n");
	cdev_init(&vga_out->cdev, &vga_out_fops);
	vga_out->cdev.owner = THIS_MODULE;
	vga_out->cdev.ops = &vga_out_fops;
	err = cdev_add(&vga_out->cdev, devno, 1);
	if (err)
		printk(KERN_NOTICE " Error %d adding vga_out%d", err,
		       index);
}


static int __devinit
vga_out_alloc(struct device *dev, int id,
	      resource_size_t hw_phy_base, resource_size_t hw_length,
	      int irq)
{
	struct Xps_VGA_out_device *vga_out;
	int rc;

	dev_dbg(dev, "vga_out_alloc(%p)\n", dev);

	if (!hw_phy_base) {
		rc = -ENODEV;
		goto err_noreg;
	}

	/* Allocate and initialize the vga_out device structure */
	vga_out = kzalloc(sizeof(struct Xps_VGA_out_device), GFP_KERNEL);
	if (!vga_out) {
		rc = -ENOMEM;
		goto err_alloc;
	}
	vga_out->dev = dev;
	vga_out->id = id;
	vga_out->hw_phy_base = (void *) hw_phy_base;
	vga_out->hw_length = hw_length;
	vga_out->irq = irq;

	/* Call the setup code */
	rc = vga_out_setup(vga_out);

	if (rc)
		goto err_setup;
	dev_set_drvdata(dev, vga_out);
	vga_out_setup_cdev(vga_out, 0);

	return 0;

      err_setup:
	kfree(vga_out);
      err_alloc:
      err_noreg:
	dev_err(dev, "could not initialize device, err=%i\n", rc);
	return rc;
}


static void __devexit vga_out_free(struct device *dev)
{
	struct Xps_VGA_out_device *vga_out = dev_get_drvdata(dev);
	dev_dbg(dev, "vga_out_free(%p)\n", dev);

	if (vga_out) {
		vga_out_teardown(vga_out);
		kfree(vga_out);
	}
}



/* ---------------------------------------------------------------------
 * Platform Bus Support
 */

static int __devinit vga_out_probe(struct platform_device *dev)
{
	resource_size_t hw_phy_base = 0, hw_length = 0;
	int id = dev->id;
	int irq = NO_IRQ;
	int i;

	dev_dbg(&dev->dev, "vga_out_probe(%p)\n", dev);

	for (i = 0; i < dev->num_resources; i++) {

		if (dev->resource[i].flags & IORESOURCE_IRQ)
			irq = dev->resource[i].start;
	}

	/* Call the bus-independant setup code */
	return vga_out_alloc(&dev->dev, id, hw_phy_base, hw_length, irq);
}

/*
 * Platform bus remove() method
 */
static int __devexit vga_out_remove(struct platform_device *dev)
{
	vga_out_free(&dev->dev);
	return 0;
}

static struct platform_driver vga_out_platform_driver = {
	.probe = vga_out_probe,
	.remove = __devexit_p(vga_out_remove),
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "vga-out",
		   },
};

/* ---------------------------------------------------------------------
 * OF_Platform Bus Support
 */

#ifdef CONFIG_OF
static int __devinit
vga_out_of_probe(struct of_device *op, const struct of_device_id *match)
{
	struct resource res;
	resource_size_t hw_phy_base = 0, hw_length = 0;
	const u32 *id;
	int irq, rc;
	const u32 *tree_info;


	dev_dbg(&op->dev, "vga_out_of_probe(%p, %p)\n", op, match);

	/* device id */
	id = of_get_property(op->node, "port-number", NULL);
	if (id == NULL)
		printk("vga_out: No id\n");
	/* physaddr */
	rc = of_address_to_resource(op->node, 0, &res);
	if (rc) {
		dev_err(&op->dev, "invalid address\n");
		return rc;
	}
	hw_phy_base = res.start;
	hw_length = res.end + 1 - res.start;

	/* irq */
	// irq = irq_of_parse_and_map(op->node, 0);
	irq = of_irq_to_resource(op->node, 0, &res);
	printk("vga_out: irq 0x%x\n", irq);


	printk("vga_out: of register : reg_phy 0x%x (0x%x) irq :0x%x\n",
	       hw_phy_base, hw_length, irq);


	/* Call the bus-independant setup code */
	return vga_out_alloc(&op->dev, id ? *id : 0,
			     hw_phy_base, hw_length, irq);
}

static int __devexit vga_out_of_remove(struct of_device *op)
{
	vga_out_free(&op->dev);
	return 0;
}

/* Match table for of_platform binding */
static struct of_device_id vga_out_of_match[] __devinitdata = {
	{.compatible = "xlnx,xps-vga-out-1.00.a",},
	{},
};

MODULE_DEVICE_TABLE(of, vga_out_of_match);

static struct of_platform_driver vga_out_of_driver = {
	.owner = THIS_MODULE,
	.name = "vga-out",
	.match_table = vga_out_of_match,
	.probe = vga_out_of_probe,
	.remove = __devexit_p(vga_out_of_remove),
	.driver = {
		   .name = "vga-out",
		   },
};

/* Registration helpers to keep the number of #ifdefs to a minimum */
static inline int __init vga_out_of_register(void)
{
	pr_debug("vga_out: registering OF binding\n");
	return of_register_platform_driver(&vga_out_of_driver);
}

static inline void __exit vga_out_of_unregister(void)
{
	of_unregister_platform_driver(&vga_out_of_driver);
}
#else				/* CONFIG_OF */
/* CONFIG_OF not enabled; do nothing helpers */
static inline int __init vga_out_of_register(void)
{
	return 0;
}

static inline void __exit vga_out_of_unregister(void)
{
}
#endif				/* CONFIG_OF */

/* ---------------------------------------------------------------------
 * Module init/exit routines
 */


static int __init vga_out_init(void)
{
	int rc;
	dev_t dev = 0;


	if (vga_out_major != 0) {
		dev = MKDEV(vga_out_major, vga_out_minor);
		rc = register_chrdev_region(dev, vga_out_nr_devs,
					    "vga-out");
	} else {
		rc = alloc_chrdev_region(&dev, vga_out_minor,
					 vga_out_nr_devs, "vga-out");
		vga_out_major = MAJOR(dev);
	}

	if (rc < 0) {
		rc = -ENOMEM;
		goto err_chr;
	}
	rc = vga_out_of_register();
	if (rc)
		goto err_of;

	pr_debug("vga_out: registering platform binding\n");
	rc = platform_driver_register(&vga_out_platform_driver);
	if (rc)
		goto err_plat;

	pr_info("vga_out: device driver, major=%i\n", vga_out_major);
	return 0;

      err_plat:
	vga_out_of_unregister();
      err_of:
	unregister_chrdev(vga_out_major, "vga-out");
      err_chr:
	printk(KERN_ERR "vga_out: registration failed; err=%i\n", rc);
	return rc;
}

static void __exit vga_out_exit(void)
{
	pr_debug("Unregistering vga_out driver\n");
	platform_driver_unregister(&vga_out_platform_driver);
	vga_out_of_unregister();
	unregister_chrdev(vga_out_major, "vga-out");
}


module_init(vga_out_init);
module_exit(vga_out_exit);

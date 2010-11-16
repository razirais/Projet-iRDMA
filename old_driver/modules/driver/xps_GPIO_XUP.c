

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
  volatile  unsigned   int GIE; 
  volatile  unsigned   int ISR;
  volatile  unsigned   int ISR_b;
  volatile  unsigned   int IER;
} Xps_GPIO_XUP_Interrupt;

typedef struct Xps_GPIO_XUP_Control{
  volatile  unsigned   int data;
  volatile  unsigned   int tristate_control;
} Xps_GPIO_XUP_Control;
#define XGPIO_TRI_ALL_OUT	0x00u
#define XGPIO_TRI_ALL_IN	0x1fu

struct Xps_GPIO_XUP_device {
  volatile  void * hw_phy_base;
  volatile  int hw_length;
  volatile  void* hw_base;

  volatile Xps_GPIO_XUP_Interrupt*it;
  volatile Xps_GPIO_XUP_Control*control;

  wait_queue_head_t wait;

  int is_in, is_out;

  int irq_present;
  int irq;
  struct device *dev;
  int id;
  struct cdev cdev;
  struct device_node *node;
} ;

static int gpio_xup_major=0;
static int gpio_xup_minor=0;
static int gpio_xup_nr_devs=3;
static int gpio_xup_dev=0;
#  define SYNCHRONIZE_IO __asm__ volatile ("eieio") /* should be 'mbar' ultimately */


MODULE_LICENSE("GPL");


/* --------- *
 * Interrupt *
 * --------- */

static irqreturn_t xps_GPIO_XUP_isr(int irq, void *dev_id) 
{
  struct Xps_GPIO_XUP_device *gpio_xup = dev_id;
  printk("I");
  gpio_xup->it->GIE=0;SYNCHRONIZE_IO;
  gpio_xup->it->ISR=1;SYNCHRONIZE_IO;

  wake_up_interruptible(&gpio_xup->wait);
  return IRQ_HANDLED;
}

static void xps_GPIO_XUP_IRQ_reset(struct Xps_GPIO_XUP_device *gpio_xup)
{
  gpio_xup->it->GIE=0;
  gpio_xup->it->IER=0;
  gpio_xup->it->ISR=0;
}
static void xps_GPIO_XUP_IRQ_init(struct Xps_GPIO_XUP_device *gpio_xup)
{
  gpio_xup->it->GIE=0xFFFFFFFF;
  gpio_xup->it->IER=0xFFFFFFFF;
  gpio_xup->it->ISR=0;
}
/* ---------------- *
 * Character driver *
 * ---------------- */


static ssize_t xps_GPIO_XUP_read(
				 struct file *filep,
				 char *buf,
				 size_t count,
				 loff_t *offset) 
{
  struct Xps_GPIO_XUP_device *gpio_xup=filep->private_data;
  char data;

  printk("gpio_xup: read\n");
  if (gpio_xup->irq_present)
    {
      /* arme les interruptions */
      xps_GPIO_XUP_IRQ_init(gpio_xup);
      if ( !(filep->f_flags & O_NONBLOCK)) 
	{
	  printk("gpio_xup: wait IT");
	  interruptible_sleep_on(&gpio_xup->wait);
	  printk("gpio_xup: IT passed");
          /* arrête les interruptions */
	  gpio_xup->it->ISR=0;
	}
    }
  data=((char)gpio_xup->control->data) & 0x0F;
  printk("gpio_xup: read data 0x%x\n",(unsigned int)data);
  copy_to_user(buf,&data,1);
  *offset += 1;
  return 1;
}


static ssize_t xps_GPIO_XUP_write(struct file *filep,
				  const char *buf,
				  size_t count,
				  loff_t *offset
				  ) {
  struct Xps_GPIO_XUP_device *gpio_xup=filep->private_data;
  char data;
  printk("gpio_xup: write\n");
  copy_from_user(&data,buf,1);
  printk("gpio_xup: write data0x%x\n", (unsigned int) data);
  if (gpio_xup->is_out)
    gpio_xup->control->data=(unsigned int)data;
  (*offset) += 1;
  return 1;
}



static int xps_GPIO_XUP_open(struct inode *inode, struct file *filep) 
{
  struct  Xps_GPIO_XUP_device*gpio_xup;
  printk("gpio_xup: open\n");
  gpio_xup=container_of(inode->i_cdev,  struct Xps_GPIO_XUP_device,  cdev);
  filep->private_data=gpio_xup;
  nonseekable_open(inode,filep);
  printk("gpio_xup: open OK\n");
  return 0;
}

static int xps_GPIO_XUP_release(struct inode *inode, struct file *filep) 
{
  struct  Xps_GPIO_XUP_device*gpio_xup=filep->private_data;
  printk("gpio_xup: release\n");
  if (gpio_xup->irq_present)
  xps_GPIO_XUP_IRQ_reset(gpio_xup);
  
  return 0;
}

static struct file_operations gpio_xup_fops = {
  .read = xps_GPIO_XUP_read,
  .write = xps_GPIO_XUP_write,
  .open = xps_GPIO_XUP_open,
  .release = xps_GPIO_XUP_release,
};

/* ------ *
 * Module *
 * ------ */
/* Le reste de ce code n'est pas le plus important */
static int gpio_xup_setup(struct Xps_GPIO_XUP_device *gpio_xup) {
  int ret;
  int port;

  init_waitqueue_head(&gpio_xup->wait);
 
  port = check_mem_region(gpio_xup->hw_phy_base,gpio_xup->hw_length);
  if (port) {
    printk("gpio_xup: cannot reserve 0x%x!\n", gpio_xup->hw_phy_base);
    return port;
  } else {
    request_mem_region(gpio_xup->hw_phy_base, 
					     gpio_xup->hw_length,
					     "gpio_xup"
					     );
    printk("gpio_xup: 0x%x (hw_phy) reserved!\n", gpio_xup->hw_phy_base);
  }
  gpio_xup->hw_base= ioremap(gpio_xup->hw_phy_base, gpio_xup->hw_length);
  gpio_xup->it=(gpio_xup->hw_base+0x11C);
  gpio_xup->control=(gpio_xup->hw_base+0x0);

  printk(
         "gpio_xup: registers remapped at 0x%x (virt)\nIT : 0x%x\nCtrl 0x%x\n",
	 gpio_xup->hw_base, gpio_xup->it, gpio_xup->control
         );
  
  if (gpio_xup->irq_present)
    {
      ret = request_irq(gpio_xup->irq,		
			xps_GPIO_XUP_isr,
			IRQF_TRIGGER_RISING  | IRQF_DISABLED,
			"gpio_xup_isr",
			gpio_xup
			);

  if (ret) {
    printk("gpio_xup: failed setting IRQ\n");
  } else {
    xps_GPIO_XUP_IRQ_init(gpio_xup);
    printk("gpio_xup: ISR  has been set\n");
  }
	
    }
else
printk("gpio_xup : No IRQ\n");
  if (gpio_xup->is_in)
    gpio_xup->control->tristate_control=XGPIO_TRI_ALL_IN;
  else
    gpio_xup->control->tristate_control=XGPIO_TRI_ALL_OUT;
  printk("gpio_xup: loaded successfully\n");
  
  return 0;
}


static void __devexit gpio_xup_teardown( struct Xps_GPIO_XUP_device *gpio_xup)
{
  printk("gpio_xup: teardown...");
  xps_GPIO_XUP_IRQ_reset(gpio_xup);
  
  cdev_del(&gpio_xup->cdev);
  iounmap(gpio_xup->hw_base);
  release_mem_region((unsigned int)gpio_xup->hw_phy_base,gpio_xup->hw_length);
  if (gpio_xup->irq_present)
    free_irq(gpio_xup->irq, gpio_xup);
  printk("teardown OK\n");
}
static void gpio_xup_setup_cdev(struct Xps_GPIO_XUP_device *gpio_xup, int index)
{
  int err,devno=MKDEV(gpio_xup_major,gpio_xup_minor+index);
  printk("gpio_xup_setup_cdev\n");
  cdev_init(&gpio_xup->cdev, &gpio_xup_fops);
  gpio_xup->cdev.owner=THIS_MODULE;
  gpio_xup->cdev.ops=&gpio_xup_fops;
  err=cdev_add(&gpio_xup->cdev, devno, 1);
  printk("gpio_xup: device %s of type 0x%x\n", gpio_xup->dev->init_name, gpio_xup->dev->type );
  printk("gpio_xup: major 0x%x minor 0x%x input 0x%x output 0x%x\n",
	 MAJOR(gpio_xup->cdev.dev), MINOR(gpio_xup->cdev.dev),
	 gpio_xup->is_in, gpio_xup->is_out);
  if (err)
    printk(KERN_NOTICE " Error %d adding gpio_xup%d",err,index);
}


static int __devinit gpio_xup_alloc(struct device *dev, int id, 
				    resource_size_t hw_phy_base, resource_size_t hw_length,
				    int is_in, int is_out,
				    int irq_present, int irq
				    )
{
  struct Xps_GPIO_XUP_device *gpio_xup;
  int rc;

  dev_dbg(dev, "gpio_xup_alloc(%p)\n", dev);

  if (!hw_phy_base) {
    rc = -ENODEV;
    goto err_noreg;
  }

  /* Allocate and initialize the gpio_xup device structure */
  gpio_xup = kzalloc(sizeof(struct Xps_GPIO_XUP_device), GFP_KERNEL);
  if (!gpio_xup) {
    rc = -ENOMEM;
    goto err_alloc;
  }
  gpio_xup->dev = dev;
  gpio_xup->id = id;
  gpio_xup->hw_phy_base = (void *)hw_phy_base;
  gpio_xup->hw_length =hw_length;
  gpio_xup->is_in = is_in;
  gpio_xup->is_out = is_out;
  gpio_xup->irq_present = irq_present;
  gpio_xup->irq = irq;
  
  /* Call the setup code */
  rc = gpio_xup_setup(gpio_xup);

  if (rc)
    goto err_setup;
  dev_set_drvdata(dev,gpio_xup);
  gpio_xup_setup_cdev(gpio_xup, gpio_xup_dev);
  gpio_xup_dev++;

  return 0;

 err_setup:
  kfree(gpio_xup);
 err_alloc:
 err_noreg:
  dev_err(dev, "could not initialize device, err=%i\n", rc);
  return rc;
}


static void __devexit gpio_xup_free(struct device *dev)
{
  struct Xps_GPIO_XUP_device *gpio_xup = dev_get_drvdata(dev);
  dev_dbg(dev, "gpio_xup_free(%p)\n", dev);

  if (gpio_xup) {
    gpio_xup_teardown(gpio_xup);
    kfree(gpio_xup);
  }
}



/* ---------------------------------------------------------------------
 * Platform Bus Support
 */

static int __devinit gpio_xup_probe(struct platform_device *dev)
{
  resource_size_t  hw_phy_base= 0,hw_length=0;
  int id = dev->id;
  int irq = NO_IRQ;
  int irq_present=0;
  int i;

  dev_dbg(&dev->dev, "gpio_xup_probe(%p)\n", dev);

  for (i = 0; i < dev->num_resources; i++) {
    
    if (dev->resource[i].flags & IORESOURCE_IRQ)
      irq = dev->resource[i].start;
  }

  /* Call the bus-independant setup code */
  return gpio_xup_alloc(&dev->dev, id,
			hw_phy_base, hw_length,
			0, 0,
			irq_present, irq);
}

/*
 * Platform bus remove() method
 */
static int __devexit gpio_xup_remove(struct platform_device *dev)
{
  gpio_xup_free(&dev->dev);
  return 0;
}

static struct platform_driver gpio_xup_platform_driver = {
  .probe = gpio_xup_probe,
  .remove = __devexit_p(gpio_xup_remove),
  .driver = {
    .owner = THIS_MODULE,
    .name = "xps-gpio",
  },
};

/* ---------------------------------------------------------------------
 * OF_Platform Bus Support
 */

#ifdef CONFIG_OF
static int __devinit
gpio_xup_of_probe(struct of_device *op, const struct of_device_id *match)
{
  struct resource res;
  resource_size_t hw_phy_base= 0,hw_length=0;
  int is_in, is_out, wd;
  u32 *id;
  int irq_present;
  
  int irq, rc;
  u32 *tree_info;


  dev_dbg(&op->dev, "gpio_xup_of_probe(%p, %p)\n", op, match);

  /* device id */
  id = of_get_property(op->node, "port-number", NULL);
  if (id==NULL)
    printk("gpio_xup: No id\n"); 
  /* physaddr */
  rc = of_address_to_resource(op->node, 0, &res);
  if (rc) {
    dev_err(&op->dev, "invalid address\n");
    return rc;
  }
  hw_phy_base = res.start;
  hw_length= res.end+1-res.start;
  /*  -- */ 
  id=of_get_property(op->node, "xlnx,gpio-width", NULL);
  if (id==NULL) {
    dev_err(&op->dev, "invalid 'xlnx,gpio-width'\n");
    return id;
  }
  wd=*id;
  printk("gpio_xup gpio-width=%d\n",wd);
  /*  -- */ 
  id=of_get_property(op->node, "xlnx,all-inputs", NULL);
  if (id==NULL) {
    dev_err(&op->dev, "invalid 'xlnx,all-inputs'\n");
    return -1;
  }
  is_in=is_out=0;
  if (*id!=0)
    is_in=wd;
  else
    is_out=wd;
  printk("gpio_xup in=0x%x out=0x%x\n", is_in, is_out);

  /* irq */
  id = of_get_property(op->node, "xlnx,interrupt-present", NULL);
  if (id==NULL){
    dev_err(&op->dev, "invalid 'xlnx,interrupt-present'\n");
    return -1;
  }
  irq_present=*id;

  if (irq_present != 0)
    {
      irq = of_irq_to_resource(op->node, 0, &res);
      printk("gpio_xup: irq 0x%x\n",irq);
    }
  
  printk("gpio_xup: of register : reg_phy 0x%x (0x%x) irq :0x%x\n",
         hw_phy_base, hw_length,irq);


  /* Call the bus-independant setup code */
  return gpio_xup_alloc(&op->dev, id ? *id : 0, 
			hw_phy_base, hw_length,
			is_in, is_out,
			irq_present, irq);
}

static int __devexit gpio_xup_of_remove(struct of_device *op)
{
  gpio_xup_free(&op->dev);
  return 0;
}

/* Match table for of_platform binding */
static struct of_device_id gpio_xup_of_match[] __devinitdata = {
  { .compatible = "xlnx,xps-gpio-1.00.a", },
  {},
};

MODULE_DEVICE_TABLE(of, gpio_xup_of_match);

static struct of_platform_driver gpio_xup_of_driver = {
  .owner = THIS_MODULE,
  .name = "xps-gpio",
  .match_table = gpio_xup_of_match,
  .probe = gpio_xup_of_probe,
  .remove = __devexit_p(gpio_xup_of_remove),
  .driver = {
    .name = "xps-gpio",
  },
};

/* Registration helpers to keep the number of #ifdefs to a minimum */
static inline int __init gpio_xup_of_register(void)
{
  pr_debug("gpio_xup: registering OF binding\n");
  return of_register_platform_driver(&gpio_xup_of_driver);
}

static inline void __exit gpio_xup_of_unregister(void)
{
  of_unregister_platform_driver(&gpio_xup_of_driver);
}
#else /* CONFIG_OF */
/* CONFIG_OF not enabled; do nothing helpers */
static inline int __init gpio_xup_of_register(void) { return 0; }
static inline void __exit gpio_xup_of_unregister(void) { }
#endif /* CONFIG_OF */

/* ---------------------------------------------------------------------
 * Module init/exit routines
 */


static int __init gpio_xup_init(void)
{
  int rc;
  dev_t dev = 0;
  
 
  if(gpio_xup_major!=0){
    dev = MKDEV(gpio_xup_major, gpio_xup_minor);
    rc = register_chrdev_region(dev, gpio_xup_nr_devs, "xps-gpio");
  }else{
    rc = alloc_chrdev_region(&dev, gpio_xup_minor, gpio_xup_nr_devs, "xps-gpio");
    gpio_xup_major=MAJOR(dev);
  }

  if (rc < 0) {
    rc = -ENOMEM;
    goto err_chr;
  }
  rc = gpio_xup_of_register();
  if (rc)
    goto err_of;

  pr_debug("gpio_xup: registering platform binding\n");
  rc = platform_driver_register(&gpio_xup_platform_driver);
  if (rc)
    goto err_plat;

  pr_info("gpio_xup device driver, major=%i\n", gpio_xup_major);
  return 0;

 err_plat:
  gpio_xup_of_unregister();
 err_of:
  unregister_chrdev(gpio_xup_major, "xps-gpio");
 err_chr:
  printk(KERN_ERR "gpio_xup: registration failed; err=%i\n", rc);
  return rc;
}

static void __exit gpio_xup_exit(void)
{
  pr_debug("Unregistering gpio_xup driver\n");
  platform_driver_unregister(&gpio_xup_platform_driver);
  gpio_xup_of_unregister();
  unregister_chrdev(gpio_xup_major, "xps-gpio");
}


module_init(gpio_xup_init);
module_exit(gpio_xup_exit);

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include "key_drv.h"
#include <linux/interrupt.h>
#include <linux/sched.h>


#define DEVICE_NAME "KEY_DRV"
static unsigned int key_major = 0; /* 0 danymic alloc */

static struct class *keys_class;
static struct device      *keys_class_dev;
static unsigned long gpio_va;

#define GPIO_OFT(x) ((x) - 0x56000000)
#define GPFCON  (*(volatile unsigned long *)(gpio_va + GPIO_OFT(0x56000010)))
#define GPFDAT  (*(volatile unsigned long *)(gpio_va + GPIO_OFT(0x56000014)))

#define GPGCON  (*(volatile unsigned long *)(gpio_va + GPIO_OFT(0x56000060)))
#define GPGDAT  (*(volatile unsigned long *)(gpio_va + GPIO_OFT(0x56000064)))

static unsigned char key_val;
struct key_desc key_desc[4] = {
	{0, 0x01},
	{3, 0x02},
	{5, 0x03},
	{6, 0x04},
};

static volatile int ev_press = 0;
struct key_dev dev;


/* interrupt routine */
static irqreturn_t keys_irq(int irq,void *dev_id)
{
	struct key_desc *desc = (struct key_desc *) dev_id;
	unsigned int val;
	val = GPGDAT & (1 << (desc->key));

	if (val ) {
		key_val = 0x80 | desc->key_val;
	} else {
		key_val = desc->key_val;
	}

	/*printk ("IRQ routine..%d \n",key_val); */
	ev_press = 1; 
	wake_up_interruptible(&dev.inq);
	return IRQ_RETVAL(IRQ_HANDLED);
 	
}

static ssize_t s3c24xx_keys_open(struct inode *inode, struct file *file)
{
	 GPFCON &= ~(0x3<<(5*2));
        GPFCON |= (1<<(5*2));

        GPFCON &= ~(0x3<<(6*2));
        GPFCON |= (1<<(6*2));

        GPFCON &= ~(0x3<<(7*2));
        GPFCON |= (1<<(7*2));

        GPFCON &= ~(0x3<<(8*2));
        GPFCON |= (1<<(8*2));

        GPFDAT |= (1<<5);
        GPFDAT |= (1<<6);
        GPFDAT |= (1<<7);
        GPFDAT |= (1<<8);
		
	return 0;
}

static ssize_t s3c24xx_keys_read(struct file *filp, char __user *buff,
                                         size_t count, loff_t *offp)
{
	if (count != 1)
		return -EINVAL;
	wait_event_interruptible(dev.inq,ev_press);
	if(copy_to_user(buff,&key_val,1))
		return -EFAULT;
	ev_press = 0;
	printk("key_valu = %.X\n", key_val);
	return 1;
}
static ssize_t s3c24xx_keys_write(struct file *file, const char __user *buf, size_t count, loff_t * ppos)
{
	return 0;
}
/* To indicate which leds will be lighten */
static long s3c24xx_keys_ioctl(struct file *filp,
                 unsigned int cmd, unsigned long arg)
{
	long retval = 0;
 	int err = 0;
	int led_number = 0;
    	if (_IOC_TYPE(cmd) != LEDS_IOC_MAGIC)
        	return -ENOTTY;
    	if (_IOC_NR(cmd) > LEDS_IOC_MAXNR)
        	return -ENOTTY;

    	if (_IOC_DIR(cmd) & _IOC_READ)
        	err =
            		!access_ok(VERIFY_WRITE, (void __user *) arg, _IOC_SIZE(cmd));
    	else if (_IOC_DIR(cmd) & _IOC_WRITE)
        	err = !access_ok(VERIFY_READ, (void __user *) arg, _IOC_SIZE(cmd));
    	if (err)
        	return -EFAULT;
	

	switch (cmd) {
    		case LEDS_IOCRESET:
		/* reset all LED register, put LED off */
	        	GPFDAT |= (1<<5) | (1 << 6) | (1 << 7) | (1 << 8);
	
			printk("LEDS_IOCRESET\n");
        	break;
    		/* interfact for KIT SET */
   		case LEDS_IOCSSETKIT:
			printk("LEDS_IOCSSETKIT\n");
        		break;
    /* interfact for LED test */
    		case LEDS_IOCSSETLEDON:
			/*retval = get_user(led_number,(int __user *) arg);*/
			led_number = arg;
			if ( led_number< 0 || led_number> 4) 
				return ENOTTY;
			printk("LEDS_IOCSSETLEDON LED%d\n",led_number);
			GPFDAT &= ~( 1 << (led_number +4)); 			
        		break;
    		case LEDS_IOCSSETLEDOFF:
        		led_number = arg;
			if ( led_number< 0 || led_number> 4) 
				return ENOTTY;
			printk("LEDS_IOCSSETLEDOFF LED%d\n",led_number);
			GPFDAT |= ( 1 << (led_number +4)); 			
			break;
    		default:
        		return -ENOTTY;
   	 }

	return retval;
}

struct file_operations s3c24xx_keys_fops = {
	.owner	= 	THIS_MODULE,
	.open	= 	s3c24xx_keys_open,
	.read 	= 	s3c24xx_keys_read,
	.write 	= 	s3c24xx_keys_write,
	.unlocked_ioctl 	= s3c24xx_keys_ioctl,
};

static int __init s3c24xx_keys_drv_init(void)
{
	/* request irq */

	int ret;
	ret = 	request_irq(IRQ_EINT8,  keys_irq, IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING, "S1", &key_desc[0]);
	if (ret !=0) {
		printk("ERROR: Cannot request IRQ = %d",IRQ_EINT8);
		printk("- Code %d, EIO %d, EINVAL %d\n",ret,EIO,EINVAL);
	}
	ret = request_irq(IRQ_EINT11, keys_irq, IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING, "S2", &key_desc[1]);
	if (ret !=0) {
		printk("ERROR: Cannot request IRQ = %d",IRQ_EINT11);
		printk("- Code %d, EIO %d, EINVAL %d\n",ret,EIO,EINVAL);
	}
	ret =request_irq(IRQ_EINT13, keys_irq, IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING, "S3", &key_desc[2]);
	if (ret !=0) {
		printk("ERROR: Cannot request IRQ = %d",IRQ_EINT13);
		printk("- Code %d, EIO %d, EINVAL %d\n",ret,EIO,EINVAL);
	}
	ret =request_irq(IRQ_EINT14, keys_irq, IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING, "S4", &key_desc[3]);
	if (ret !=0) {
		printk("ERROR: Cannot request IRQ = %d",IRQ_EINT14);
		printk("- Code %d, EIO %d, EINVAL %d\n",ret,EIO,EINVAL);
	}



	/* remap GPIO */
  	if (! request_region(0x56000000, 0x400, "keys-drv")) {
                        printk(KERN_INFO "s3c24xx: can't get I/O port address 0x%x\n",
                                        0x56000000);
                        return -ENODEV;
                }

    	gpio_va = (unsigned long) ioremap(0x56000000, 0x400);
        if (!gpio_va) {
                return -EIO;
        }
	
	printk("s3c24xx keys drv init\n");	
	key_major = register_chrdev(0,DEVICE_NAME, &s3c24xx_keys_fops);
	
	if (key_major < 0 ) {
		printk(DEVICE_NAME "Can't register major number\n");
		return key_major;
	}

	keys_class = class_create(THIS_MODULE,"keys-drv");
	if (IS_ERR(keys_class)) {
		printk(DEVICE_NAME "class create failure\n");
		return PTR_ERR(keys_class);
	}
	
	keys_class_dev = device_create(keys_class,NULL,MKDEV(key_major,0),NULL,"keys-drv"); 
	if (unlikely(IS_ERR(keys_class_dev))) {
		return PTR_ERR(keys_class_dev);
	}	
	
	init_waitqueue_head(&dev.inq);
	return 0;
}

static void __exit s3c24xx_keys_drv_exit(void)
{
	dev_t devno = MKDEV(key_major, 0);
	unregister_chrdev(key_major,DEVICE_NAME);
	device_destroy(keys_class,devno);
	class_destroy(keys_class);
	iounmap((unsigned long *)gpio_va);
	release_region(0x56000000, 0x400);
	printk(" keys drv exit \n");
}


module_init(s3c24xx_keys_drv_init);
module_exit(s3c24xx_keys_drv_exit);

MODULE_AUTHOR("James");
MODULE_VERSION("0.1.0");
MODULE_DESCRIPTION("S3C2440 LED Driver");
MODULE_LICENSE("GPL");



#ifndef _LEDS_DRV_H_
#define _LEDS_DRV_H_

#include <linux/ioctl.h>  /* need for the _IOW etc stuff used later */

struct key_desc {
	unsigned int key;
	unsigned int key_val;
};

struct key_dev {
      	 wait_queue_head_t inq;       /* read queues */
        struct mutex mutex;              /* mutual exclusion semaphore */
	 int key_press_flag;
};

#define LEDS_IOC_MAGIC 'G'

#define LEDS_IOCRESET _IO(LEDS_IOC_MAGIC,0)
/* S means SET  */

#define LEDS_IOCSSETKIT _IOW(LEDS_IOC_MAGIC,1,int)
#define LEDS_IOCSSETLEDON _IOW(LEDS_IOC_MAGIC,2,int)   /* for test ioctl function and */
#define LEDS_IOCSSETLEDOFF _IOW(LEDS_IOC_MAGIC,3,int)   /* for test ioctl function and */
#define LEDS_IOC_MAXNR 3   /* user this check number of operation */



#endif

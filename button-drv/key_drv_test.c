#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>

#define LEDS_IOC_MAGIC 'G'
#define LEDS_IOCRESET _IO(LEDS_IOC_MAGIC,0)
/* S means SET  */
#define LEDS_IOCSSETKIT _IOW(LEDS_IOC_MAGIC,1,int)
#define LEDS_IOCSSETLED _IOW(LEDS_IOC_MAGIC,2,int)   /* for test ioctl function and */
#define LEDS_IOCSSETLEDON _IOW(LEDS_IOC_MAGIC,2,int)   /* for test ioctl function and */
#define LEDS_IOCSSETLEDOFF _IOW(LEDS_IOC_MAGIC,3,int)   /* for test ioctl function and */
#
int main(int argc, char **argv)
{
	int fd;
	char *filename;
	char val;
	int i = 0 ; 
	unsigned char key_press;

	if (argc != 2) {
		printf("usage: %s filename");	
		return 0;
	}

	filename = argv[1];

	fd = open(filename, O_RDWR);
	if (fd < 0) {
		printf("error, can't open %s\n", filename);
		return 0;
	}

	/* ioctl */
	val = ioctl(fd, LEDS_IOCRESET);
	if ( val  != 0 ) {
		printf("error , ioctl %d\n", val);
	}

	for ( ;; i++) {
				
		read(fd,&key_press,1);
		if (key_press > 0x80) {
			printf("key value = %.X\n", key_press);
			val = ioctl(fd, LEDS_IOCSSETLEDOFF,(key_press - 0x80) );
			if ( val  != 0 ) {
				printf("error , ioctl %d\n", val);
			}
		} else {
			val = ioctl(fd, LEDS_IOCSSETLEDON, key_press);
			if ( val  != 0 ) {
				printf("error , ioctl %d\n", val);

			}
		}
	}

	close(fd);

}

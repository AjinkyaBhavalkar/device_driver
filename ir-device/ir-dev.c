/*
 * Describes an IR Blaster (for Casio Remote) at GPIO 23
 * 
 */

#include <linux/module.h>
#include <linux/errno.h>

#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/time.h>
#include <linux/timex.h>
#include <linux/timekeeping.h>
#include <linux/delay.h>

#ifndef MAX_UDELAY_MS
#define MAX_UDELAY_US 5000
#else
#define MAX_UDELAY_US (MAX_UDELAY_MS*1000)
#endif

#define KEY_POWER	0x212FD02F
#define NUMBITS 	32
#define PERIOD 		26315
#define PULSE_WIDTH	7894
#define SPACE_WIDTH	18421

/* GPIO Pin Hardcoded */
static int gpio_out_pin = 23;

static bool softcarrier = 1;
static struct gpio_chip *gpiochip;

static long send_pulse(unsigned long length);
static void send_space(long length);

module_param(gpio_out_pin, int, 0);
MODULE_PARM_DESC(gpio_out_pin, "GPIO output pin");
	
/* GPIO */
static int is_right_chip(struct gpio_chip *chip, void *data)
{
	if (strcmp(data, chip->label) == 0)
		return 1;
	return 0;
}

/* Set Pin Direction */
static void inline gpio_setpin(int pin, int value)
{
	gpiochip->set(gpiochip, pin, value);
	
}

static void safe_udelay(unsigned long usecs)
{
	while (usecs > MAX_UDELAY_US) 
	{
		udelay(MAX_UDELAY_US);
		usecs -= MAX_UDELAY_US;
	}
	udelay(usecs);
}

static unsigned long read_current_us(void)
{
	struct timespec now;
	getnstimeofday(&now);
	return (now.tv_sec * 1000000)+(now.tv_nsec/1000);
}


static long send_pulse_softcarrier(unsigned long length)
{
	unsigned long actual_us=0, initial_us=0;
	length *= 1000;
	initial_us = read_current_us();
	while(actual_us < length){
			gpio_setpin(gpio_out_pin, 1);
			udelay(8);

			gpio_setpin(gpio_out_pin, 0);
			udelay(17);

		actual_us = (read_current_us()-initial_us)*1000;
	}
	return 0;
}


static long send_pulse(unsigned long length)
{
	if(length <= 0)
		return 0;
	else
	{
		gpio_setpin(gpio_out_pin,1);
		safe_udelay(length);
	}
}

static void send_space(long length)
{	
	gpio_setpin(gpio_out_pin, 0);
	if (length <= 0)
		return;
	safe_udelay(length);
}

static void sendNEC(long data, int nbits){
	int i=0;
	//safe_udelay(10000);
	//printk("%lu\n",read_current_us());
	send_pulse(9000);
	send_space(4500);
	for(i=0;i<nbits;i++)
	{
		if(data & 0x80000000)
			{
			send_pulse(560);
			send_space(1600);
			//printk("1");
			}
		else
			{
			send_pulse(560);
			send_space(560);
			//printk("0");
			}
		data = data<<1; //shift data left by 1 bit
	}
	send_pulse(560); //at the end send the stop signal
	send_space(0);
	//printk("\n");
}


/* 1 Define device specific data */
struct ir_data {
	char *name;
	int status;
};
#define to_ir_data(x)	((struct ir_data *)((x)->platform_data))

/* Attributes */
/* 2 define show and store for each attribute */
static ssize_t name_show(struct device *dev, struct device_attribute *attr, char *resp)
{
	struct platform_device *pdev = to_platform_device(dev);

	return snprintf(resp, 40, "%s\n", pdev->name);
}

static struct device_attribute dev_attr_name = {
	.attr = {
		.name = "name", /*sys filename */
		.mode = 0644, /* permissions */
	},
	.show = name_show,	/* show function */
};

static ssize_t status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t result;
	struct ir_data *pdev = to_ir_data(dev);
	result = sprintf(buf, "status = %d\n", pdev->status);
	return result;

}

static ssize_t status_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t valsize)
{

	struct ir_data *pdev = to_ir_data(dev);
	
	long value;
	ssize_t result;

	result = sscanf(buf, "%ld", &value); 
	if(result != 1) return -EINVAL;

	else
	{
			sendNEC(KEY_POWER, NUMBITS);
			result = valsize;
	}
	
	return result;
}		

static struct device_attribute dev_attr_status = {
	.attr = {
		.name = "status", /*sys filename */
		.mode = 0644, /* permissions */
	},
	.show = status_show,	/* show function */
	.store = status_store,
};	


//static DEVICE_ATTR(status, (S_IRWXU | S_IRWXG),status_show, status_store);

/* 3 build groups of attributes */
static struct attribute *ir_basic_attrs[] = {
	&dev_attr_name.attr,
	&dev_attr_status.attr,
	NULL
};

static struct attribute_group ir_basic_group = {
	.attrs = ir_basic_attrs,
};

/* 4 collect all groups */
static const struct attribute_group *ir_all_attr_groups[] = {
	&ir_basic_group,
	NULL
};
	
/* 5 Device release */
static void ir_release(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);

	dev_alert(dev,"releasing gpio device %s\n", pdev->name);

}

/* 6 define ops */
static int ir_register_device(struct platform_device *pdev)
{
	struct ir_data *gdev;
	int err;
	
	gdev = (struct ir_data *)pdev->dev.platform_data;
	pr_info ("registering device");
	err = platform_device_register(pdev);
	if (err)
		pr_err("device failed to register");
	return err;

}

static void ir_unregister_device(struct platform_device *pdev)
{
	dev_alert(&pdev->dev, "unregistering device");
	platform_device_unregister(pdev);
	//struct device *dev = &pdev->dev;
	//device_remove_file(dev, &dev_attr_name);
	//device_unregister(dev);

	
}


/* Device structure */
static struct ir_data irdata;

static struct platform_device irdev = {
	.name = "ir",
	.id = 0,
	.dev = {
		.release = ir_release,
		.groups = ir_all_attr_groups,
	},
};

static int __init ir_load(void)
{
	pr_info("loading module ir-device\n");
	irdev.dev.platform_data = &irdata;

	if (!gpiochip) { 
		gpiochip = gpiochip_find("pinctrl-bcm2835", is_right_chip);
	}	

	if(!gpiochip) {
		pr_err("gpiochip not found\n");
	}
	gpiochip->direction_output(gpiochip, gpio_out_pin, 0);


	return ir_register_device(&irdev);

}
module_init(ir_load);

static void __exit ir_unload(void)
{
	pr_info("exiting module ir-device\n");	
	ir_unregister_device(&irdev);
}

module_exit(ir_unload);

MODULE_DESCRIPTION("IR-Blaster");
MODULE_LICENSE("GPL");


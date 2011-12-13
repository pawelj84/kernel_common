/*
 *  Driver for iRex ER0100 Battery module (TI BQ2060A)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <asm/uaccess.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <asm/io.h>
#include <asm/arch/irex_er0100.h>

MODULE_AUTHOR("Milan Votava <votava@mageo.cz>");
MODULE_DESCRIPTION("iRex ER0100 battery driver");
MODULE_LICENSE("GPL");

unsigned int delta_base = 0xF8E00000;

unsigned int battery_value = 0;
unsigned int battery_pos = 0;
unsigned int battery_invalid = 0;
unsigned int battery_invalid_count = 0;
unsigned int battery_irq_enable = 0;
unsigned int previous_charge = 0;
unsigned int previous_time = 0;
unsigned int previous_temp = 0;
unsigned int previous_status = 0;
unsigned int previous_current = 0;

wait_queue_head_t irex_er0100_battery_waitq;

static int major = 223;	/* default is dynamic major device number */

module_param(major, uint, 0);
MODULE_PARM_DESC(major, "Major device number");

static void battery_write_cmd(unsigned int cmd) {
    unsigned int x;
    unsigned int cnt = 0;

    printk("battery_write_cmd %x\n", cmd);

    cmd &= 0xff;

    x = __raw_readl(delta_base + 0xc);

    battery_value = 0;
    battery_pos = 0;
    battery_invalid = 0;

    __raw_writel(x | 0x2000000, delta_base + 0xc);

    udelay(200);

    x = __raw_readl(delta_base + 0xc);
    __raw_writel(x & ~0x2000000, delta_base + 0xc);

    udelay(50);

    do {
        x = __raw_readl(delta_base + 0xc);
        __raw_writel(x | 0x2000000, delta_base + 0xc);

        udelay(10);
        
        x = cmd >> cnt;     
        if(x & 1) {
            x = __raw_readl(delta_base + 0xc);
            __raw_writel(x & ~0x2000000, delta_base + 0xc);
        }

        udelay(90);
        x = __raw_readl(delta_base + 0xc);
        cnt++;
        __raw_writel(x & ~0x2000000, delta_base + 0xc);
        udelay(100);
    } while(cnt <= 6);

    x = __raw_readl(delta_base + 0xc);
    __raw_writel(x | 0x2000000, delta_base + 0xc);

    udelay(100);

    x = __raw_readl(delta_base + 0xc);

    battery_irq_enable = 1;

    __raw_writel(x & ~0x2000000, delta_base + 0xc);
    
    if(! interruptible_sleep_on_timeout(&irex_er0100_battery_waitq, 3)) {
        printk("battery_invalid 2\n");
        battery_invalid = 1;
    }

    battery_irq_enable = 0;
}

static irqreturn_t battery_irq_handler(int irq, void *data)
{
    printk("battery_irq_handler\n");

    if(battery_irq_enable) {
        unsigned int x, x1, x2, x3, x4;

        x = __raw_readl(delta_base);
        x &= 0x2000000; 

        udelay(50);

        x1 = __raw_readl(delta_base);
        x1 &= 0x2000000; 

        udelay(5);

        x2 = __raw_readl(delta_base);
        x2 &= 0x2000000; 

        udelay(5);

        x3 = __raw_readl(delta_base);
        x3 &= 0x2000000; 

        udelay(5);

        x4 = __raw_readl(delta_base);
        x4 &= 0x2000000; 

        printk("x=%x x1=%x x2=%x x3=%x x4=%x\n", x, x1, x2, x3, x4);
        if(x == 0 && x1 == x2 && x1 == x3 && x3 == x4) {
            if(x1) {
                battery_value |= (1 << battery_pos);
            } 
        } else {
            printk("battery_invalid 1\n");
            battery_invalid = 1;
        }
        battery_pos++;

        printk("battery_pos %d\n", battery_pos);
        if((battery_pos & 0xff) == 0x10) {
            printk("Waking up the wait queue\n");
            wake_up(&irex_er0100_battery_waitq);
            //__wake_up(&irex_er0100_battery_waitq, 1, 1, NULL);
        }
    }

    return IRQ_HANDLED;
}

static int battery_open(struct inode *inode, struct file *file)
{
    printk("battery_open\n");

	return 0;
}

static int battery_release(struct inode *inode, struct file *file)
{
    printk("battery_release\n");

	return 0;
}

static int battery_ioctl(struct inode *inode, struct file *file,
            unsigned int cmd, unsigned long arg)
{
    unsigned int x;
    int ret = 0;
 
    printk("battery_ioctl %x\n", cmd);

    switch(cmd) {
    case 0x40046208:
        {
            register unsigned int r4 asm("r4");
            
            r4 = arg & 1;
            

            asm("mcr p14, 0, r4, c6, c0");
        }
        break;
    case 0x80046201:
        battery_write_cmd(0xd);
        if(battery_invalid) {
            battery_value = previous_charge;
        } else {
            previous_charge = battery_value;
        }
        if(arg) {
            if(put_user(battery_value, (unsigned int *)arg))
                return(-EFAULT);
        }
        break;
    case 0x80046202: //?loc_440
        battery_write_cmd(0x12);
        if(battery_invalid) {
            battery_value = previous_time;
        } else {
            previous_time = battery_value;
        }
        if(arg) {
            if(put_user(battery_value, (unsigned int *)arg))
                return(-EFAULT);
        }
        break;
    case 0x80046203:
        battery_write_cmd(8);
        if(battery_invalid) {
            battery_value = previous_temp;
        } else {
            previous_temp = battery_value;
        }
        if(arg) {
            if(put_user(battery_value, (unsigned int *)arg))
                return(-EFAULT);
        }
        break;
    case 0x80046204:   //loc_588
        x = __raw_readl(delta_base + 0xc);
        __raw_writel(x | 0x8000000, delta_base + 0xc);
        x = __raw_readl(delta_base + 0x18);
        __raw_writel(x | 0x8000000, delta_base + 0x18);
        break;
    case 0x80046205:
        x = __raw_readl(delta_base + 0x24);
        __raw_writel(x | 0x4000, delta_base + 0x24);
        break;
    case 0x80046206:   //?loc_570
        x = __raw_readl(delta_base + 0x18);
        __raw_writel(x | 0x4000, delta_base + 0x18);
        break;
    case 0x80046207:
        {
            register unsigned int r3 asm("r3");
            
            asm("mrc p14, 0, r3, c6, c0");
            x = r3;
        }
        if(arg) {
            if(put_user(x, (unsigned int *)arg))
                return(-EFAULT);
        }
        break;
    case 0x80046209:
        x = __raw_readl(delta_base + 0x2c);
        __raw_writel(x | 0x800, delta_base + 0x2c);
        break;
    case 0x8004620A:
        x = __raw_readl(delta_base + 0x20);
        __raw_writel(x | 0x800, delta_base + 0x20);
        break;
    case 0x8004620B:
        x = __raw_readl(delta_base + 0x24);
        __raw_writel(x | 0x80000, delta_base + 0x24);
        break;
    case 0x8004620C:
        x = __raw_readl(delta_base + 0x18);
        __raw_writel(x | 0x80000, delta_base + 0x18);
        break;
    case 0x8004620D: //loc_60c
        x = __raw_readl(delta_base + 0x28);
        __raw_writel(x | 2, delta_base + 0x28);
        break;
    case 0x8004620E:
        x = __raw_readl(delta_base + 0x1c);
        __raw_writel(x | 2, delta_base + 0x1c);
        break;
    case 0x8004620F:
        battery_write_cmd(0x16);
        if(battery_invalid) {
            battery_value = previous_status;
        } else {
            previous_status = battery_value;
        }
        if(arg) {
            if(put_user(battery_value, (unsigned int *)arg))
                return(-EFAULT);
        }
        break;
    case 0x80046210:
        battery_write_cmd(0xa);
        if(battery_invalid) {
            battery_value = previous_current;
        } else {
            previous_current = battery_value;
        }
        if(arg) {
            if(put_user(battery_value, (unsigned int *)arg))
                return(-EFAULT);
        }
        break;


    default:
        printk("battery_ioctl: Invalid command %x\n", cmd);
        return -EINVAL;
        break;
    }
//loc_690        
    if(battery_invalid) {
        battery_invalid_count++; 
        if((battery_invalid_count << 16) >= 0x40000) {
            printk("Battery: Response rejected 5 times. Results INVALID\n");
            
            previous_charge = -1;
            previous_time = -1;
            previous_temp = -1;
            previous_status = -1;
        } else {
            printk("Battery: Response rejected %d times\n", battery_invalid_count);
        }
        ret = 4;
    } else{
        battery_invalid_count = 0;
    }

    return ret;
}

static const struct file_operations battery_fops = {
	.owner		= THIS_MODULE,
    .ioctl      = battery_ioctl,
	.open		= battery_open,
	.release	= battery_release,
};

static int __devinit battery_probe(struct platform_device *dev)
{
	int retval;
    unsigned int x;

    printk("battery_probe\n");

	retval = register_chrdev(major, "battery", &battery_fops);
	if (retval < 0) {
        printk("iRex ER0100 eReader Battery Gas Gauge Driver: register_chrdev failed!\n");

		return retval;
	}

    printk("iRex ER0100 eReader Battery Gas Gauge Driver initialised\n");

	if (major == 0) {
		major = retval;
		printk(KERN_INFO "battery: major number %d\n", major);
	}

    x = __raw_readl(delta_base + 0xc);
    __raw_writel(x & ~0x2000000, delta_base + 0xc);

    init_waitqueue_head(&irex_er0100_battery_waitq);

    set_irq_type(BATTERY_IRQ, IRQT_RISING);
    if(request_irq(BATTERY_IRQ, battery_irq_handler, SA_INTERRUPT, "Battery Gas Gauge Response", NULL)<0)
    {
        printk( KERN_ERR "Request for IRQ Battery Gas Gauge Response (%u) failed\n", BATTERY_IRQ);
        return -1;
    }
	return 0;
}

static int __devexit battery_remove(struct platform_device *dev)
{
    int ret = 0;

    printk("battery_remove\n");

    free_irq(BATTERY_IRQ, NULL);

    ret = unregister_chrdev(major, "battery");
    if (ret < 0)
        printk("Error unregistering device: %d\n", ret); 


	return ret;
}

static struct platform_device *battery_platform_device;

static struct platform_driver battery_device_driver = {
	.probe		= battery_probe,
	.remove		= __devexit_p(battery_remove),
	.driver		= {
		.name	= "battery",
		.owner	= THIS_MODULE,
	},
};

static int __init battery_init(void)
{
	int retval;

    printk("battery_init\n");

	battery_platform_device = platform_device_alloc("battery", -1);
	if (!battery_platform_device)
		return -ENOMEM;

	retval = platform_device_add(battery_platform_device);
	if (retval < 0) {
		platform_device_put(battery_platform_device);
		return retval;
	}

	retval = platform_driver_register(&battery_device_driver);
	if (retval < 0)
		platform_device_unregister(battery_platform_device);

	return retval;
}

static void __exit battery_exit(void)
{
    printk("battery_exit\n");

	platform_driver_unregister(&battery_device_driver);

	platform_device_unregister(battery_platform_device);
}

module_init(battery_init);
module_exit(battery_exit);

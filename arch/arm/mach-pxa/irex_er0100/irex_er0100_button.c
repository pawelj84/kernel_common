/*
 *  Driver for iRex ER0100 Buttons
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
MODULE_DESCRIPTION("iRex ER0100 button driver");
MODULE_LICENSE("GPL");

unsigned int delta_base = 0xF8E00000;
unsigned int pendetect = 0;
unsigned int cfdetect = 0;
unsigned int mmcdetect = 0;
unsigned int button_pressed = 0;
unsigned int last_button_pressed;
unsigned int button_first_open = 1;

wait_queue_head_t irex_er0100_button_waitq;
struct timer_list wacom_off_timer;

static int major = 222;	/* default is dynamic major device number */

module_param(major, uint, 0);
MODULE_PARM_DESC(major, "Major device number");

static unsigned int scan_for_key(void) {
    unsigned int x, x1, x2, x3;
    unsigned int y;

    __raw_writel(0x40, delta_base + 0x1c);
    __raw_writel(0x20, delta_base + 0x28);
    __raw_writel(0x10, delta_base + 0x28);
    __raw_writel(8, delta_base + 0x28);

    udelay(100);

    x = __raw_readl(delta_base);
    x1 = __raw_readl(delta_base);
    x2 = __raw_readl(delta_base);
    x3 = __raw_readl(delta_base);

    __raw_writel(0x40, delta_base + 0x28);
    __raw_writel(0x20, delta_base + 0x1c);

    y = x & 4 ? 0xc : 0xff;
    if(x1 & 8)
        y = 2;      // UP/POP
    if(x2 & 0x10)
        y = 1;      // SETTINGS
    if(x3 & 0x20)
        y = 0;      // @i@

    udelay(100);

    x = __raw_readl(delta_base);
    x1 = __raw_readl(delta_base);
    x2 = __raw_readl(delta_base);
    x3 = __raw_readl(delta_base);

    __raw_writel(0x20, delta_base + 0x28);
    __raw_writel(0x10, delta_base + 0x1c);

    if(x & 4)
        y = 6;      // NOTES
    if(x1 & 8)
        y = 5;      // DOCS
    if(x2 & 0x10)
        y = 4;      // BOOKS
    if(x3 & 0x20)
        y = 3;      // NEWS

    udelay(100);

    x = __raw_readl(delta_base);
    x1 = __raw_readl(delta_base);
    x2 = __raw_readl(delta_base);
    x3 = __raw_readl(delta_base);

    __raw_writel(0x10, delta_base + 0x28);
    __raw_writel(8, delta_base + 0x1c);

    if(x & 4)
        y = 0xff;
    if(x1 & 8)
        y = 0xf;
    if(x2 & 0x10)
        y = 8;      // PREVIOUS PAGE
    if(x3 & 0x20)
        y = 7;      // NEXT PAGE

    udelay(100);

    x = __raw_readl(delta_base);
    x1 = __raw_readl(delta_base);
    x2 = __raw_readl(delta_base);
    x3 = __raw_readl(delta_base);

    __raw_writel(0x40, delta_base + 0x1c);
    __raw_writel(0x20, delta_base + 0x1c);
    __raw_writel(0x10, delta_base + 0x1c);

    if(x & 4)
        y = 0xff;
    if(x1 & 8)
        y = 0xa;    // SELECT
    if(x2 & 0x10)
        y = 0xb;    // ARROW DOWN
    if(x3 & 0x20)
        y = 9;      // ARROW UP

    printk("scan_for_key %x\n", y);
    return y;
}

static void button_press_handler(unsigned int code) {
    unsigned int key, x, y, cnt;

    printk("button_press_handler\n");

    if(jiffies - last_button_pressed) {
        cnt = 0;
        key = scan_for_key();
    
        do {
            switch(code) {
            case 0xd:
                x = __raw_readl(delta_base);
                y = x & 2 ? 0xff : 0xd;
                break;
            case 0xe:
                x = __raw_readl(delta_base);
                y = x & 0x4000000 ? 0xff : 0xe;
                break;
            case 0x10:
                x = __raw_readl(delta_base);
                y = x & 0x100000 ? 0xff : 0x10;
                break;
            case 0x11:
                x = __raw_readl(delta_base);
                y = x & 0x80 ? 0x11 : 0xff;
                break;
            default:
                y = scan_for_key();
            }
            if(key == y)
                cnt++;
            else 
                cnt = 0;
            key = y;
    
        } while(cnt <= 2);
        
        
        printk("button_pressed %x\n", y);

        button_pressed = y;
        last_button_pressed = jiffies;
    }
}

static irqreturn_t buttonpress1_irq_handler(int irq, void *data)
{
    printk("buttonpress1_irq_handler\n");

    disable_irq(BUTTON_ROW1_IRQ);
    disable_irq(BUTTON_ROW2_IRQ);
    disable_irq(BUTTON_ROW3_IRQ);
    disable_irq(BUTTON_ROW4_IRQ);

    button_press_handler(0xff);

    enable_irq(BUTTON_ROW1_IRQ);
    enable_irq(BUTTON_ROW2_IRQ);
    enable_irq(BUTTON_ROW3_IRQ);
    enable_irq(BUTTON_ROW4_IRQ);

    return IRQ_HANDLED;
}

static irqreturn_t buttonpress2_irq_handler(int irq, void *data)
{
    printk("buttonpress2_irq_handler\n");

    disable_irq(BUTTON_ROW1_IRQ);
    disable_irq(BUTTON_ROW2_IRQ);
    disable_irq(BUTTON_ROW3_IRQ);
    disable_irq(BUTTON_ROW4_IRQ);

    button_press_handler(0xff);

    enable_irq(BUTTON_ROW1_IRQ);
    enable_irq(BUTTON_ROW2_IRQ);
    enable_irq(BUTTON_ROW3_IRQ);
    enable_irq(BUTTON_ROW4_IRQ);

    return IRQ_HANDLED;
}

static irqreturn_t buttonpress3_irq_handler(int irq, void *data)
{
    printk("buttonpress3_irq_handler\n");

    disable_irq(BUTTON_ROW1_IRQ);
    disable_irq(BUTTON_ROW2_IRQ);
    disable_irq(BUTTON_ROW3_IRQ);
    disable_irq(BUTTON_ROW4_IRQ);

    button_press_handler(0xff);

    enable_irq(BUTTON_ROW1_IRQ);
    enable_irq(BUTTON_ROW2_IRQ);
    enable_irq(BUTTON_ROW3_IRQ);
    enable_irq(BUTTON_ROW4_IRQ);

    return IRQ_HANDLED;
}

static irqreturn_t buttonpress4_irq_handler(int irq, void *data)
{
    printk("buttonpress4_irq_handler\n");

    disable_irq(BUTTON_ROW1_IRQ);
    disable_irq(BUTTON_ROW2_IRQ);
    disable_irq(BUTTON_ROW3_IRQ);
    disable_irq(BUTTON_ROW4_IRQ);

    button_press_handler(0xff);

    enable_irq(BUTTON_ROW1_IRQ);
    enable_irq(BUTTON_ROW2_IRQ);
    enable_irq(BUTTON_ROW3_IRQ);
    enable_irq(BUTTON_ROW4_IRQ);

    return IRQ_HANDLED;
}

static irqreturn_t power_irq_handler(int irq, void *data)
{
    printk("power_irq_handler\n");

    button_press_handler(0xe);

    return IRQ_HANDLED;
}

static irqreturn_t pen_detect_irq_handler(int irq, void *data)
{
    unsigned int x;

    printk("pen_detect_irq_handler\n");

    udelay(1000);
    x = __raw_readl(delta_base);
    if (x & 2) {
        if(pendetect) {
            printk("Kernel: WACOM On\n");
            pendetect = 0; 

            x = __raw_readl(delta_base + 0x2c);
            __raw_writel(x | 0x800, delta_base + 0x2c);
        }
    } else {
        if(! pendetect) {
            printk("Kernel: WACOM Off\n");
            pendetect = 1;

            x = __raw_readl(delta_base + 0x20);
            __raw_writel(x | 0x800, delta_base + 0x20);
        }
    }

    return IRQ_HANDLED;
}

static irqreturn_t cf_carddetect_irq_handler(int irq, void *data)
{
    printk("cf_carddetect_irq_handler\n");

    button_press_handler(0x10);

    if(button_pressed == 0x10) {
        cfdetect = 1;
    } else {
        cfdetect = 0;
    }

    return IRQ_HANDLED;
}

static irqreturn_t mmc_carddetect_irq_handler(int irq, void *data)
{
    printk("mmc_carddetect_irq_handler\n");

    button_press_handler(0x11);

    if(button_pressed == 0x11) {
        mmcdetect = 1;
    } else {
        mmcdetect = 0;
    }
    return IRQ_HANDLED;
}

void wacom_off_delayed(unsigned long data) {
    unsigned int x;

    printk("wacom_off_delayed\n");

    x = __raw_readl(delta_base);
    
    if(! (x & 2)) {
        printk("Kernel: WACOM Off\n");
        x = __raw_readl(delta_base + 0x20);
        __raw_writel(x | 0x800, delta_base + 0x20);
    }

    enable_irq(PEN_DETECT_IRQ);
}

static int buttons_open(struct inode *inode, struct file *file)
{
    printk("buttons_open\n");

    if (button_first_open) {
        printk("Kernel: WACOM Off Delay start\n");

        init_timer(&wacom_off_timer);
        wacom_off_timer.expires = jiffies + 1000;
        wacom_off_timer.data = 0;
        wacom_off_timer.function = wacom_off_delayed;
        add_timer(&wacom_off_timer);

        button_first_open = 0;
    }

	return 0;
}

static int buttons_release(struct inode *inode, struct file *file)
{
    printk("buttons_release\n");

	return 0;
}

static int buttons_ioctl(struct inode *inode, struct file *file,
            unsigned int cmd, unsigned long arg)
{
    unsigned int x;
 
    //printk("buttons_ioctl %x\n", cmd);

    switch(cmd) {
    case 0x40046201:
        printk("buttons_ioctl %x\n", cmd);
        if(get_user(x, (unsigned int *)arg))
            return(-EFAULT);
        break;
    case 0x40046202:
        // ?led on/off
        printk("buttons_ioctl %x\n", cmd);
        if(get_user(x, (unsigned int *)arg))
            return(-EFAULT);
        if(x) {
            x = __raw_readl(delta_base + 0x18);
            __raw_writel(x | 0x800000, delta_base + 0x18);
        } else {
            x = __raw_readl(delta_base + 0x24);
            __raw_writel(x | 0x800000, delta_base + 0x24);
        }
        break;
    case 0x80046207:
    case 0x80046203:
        x = button_pressed | cfdetect << 10 | mmcdetect << 11; 
        if(put_user(x, (unsigned int *)arg))
            return(-EFAULT);
        break;
    default:
        printk("buttons_ioctl: Invalid command %x\n", cmd);
        return -EINVAL;
        break;
    }

    return 0;
}

static const struct file_operations buttons_fops = {
	.owner		= THIS_MODULE,
    .ioctl      = buttons_ioctl,
	.open		= buttons_open,
	.release	= buttons_release,
    
};

static int __devinit buttons_probe(struct platform_device *dev)
{
	int retval;
    unsigned int x;

    printk("buttons_probe\n");

	retval = register_chrdev(major, "buttons", &buttons_fops);
	if (retval < 0) {
		return retval;
	}

    printk("iRex ER0100 eReader Button Driver initialised\n");

	if (major == 0) {
		major = retval;
		printk(KERN_INFO "buttons: major number %d\n", major);
	}

    x = __raw_readl(delta_base + 0xc);
    __raw_writel(x & ~4, delta_base + 0xc);

    x = __raw_readl(delta_base + 0xc);
    __raw_writel(x & ~8, delta_base + 0xc);

    x = __raw_readl(delta_base + 0xc);
    __raw_writel(x & ~0x10, delta_base + 0xc);

    x = __raw_readl(delta_base + 0xc);
    __raw_writel(x & ~0x20, delta_base + 0xc);

    x = __raw_readl(delta_base + 0x10);
    __raw_writel(x | 0x40, delta_base + 0x10);

    x = __raw_readl(delta_base + 0x10);
    __raw_writel(x | 0x20, delta_base + 0x10);

    x = __raw_readl(delta_base + 0x10);
    __raw_writel(x | 0x10, delta_base + 0x10);

    x = __raw_readl(delta_base + 0x10);
    __raw_writel(x | 8, delta_base + 0x10);

    x = __raw_readl(delta_base + 0xc);
    __raw_writel(x & ~0x4000000, delta_base + 0xc);

    x = __raw_readl(delta_base + 0xc);
    __raw_writel(x & ~2, delta_base + 0xc);

    x = __raw_readl(delta_base + 0xc);
    __raw_writel(x & ~0x100000, delta_base + 0xc);

    x = __raw_readl(delta_base + 0xc);
    __raw_writel(x & ~0x80, delta_base + 0xc);

    x = __raw_readl(delta_base + 0xc);
    __raw_writel(x | 0x800000, delta_base + 0xc);

    x = __raw_readl(delta_base + 0x1c);
    __raw_writel(x | 0x40, delta_base + 0x1c);

    x = __raw_readl(delta_base + 0x1c);
    __raw_writel(x | 0x20, delta_base + 0x1c);

    x = __raw_readl(delta_base + 0x1c);
    __raw_writel(x | 0x10, delta_base + 0x1c);

    x = __raw_readl(delta_base + 0x1c);
    __raw_writel(x | 8, delta_base + 0x1c);

    x = __raw_readl(delta_base);
    x >>= 1;
    x ^= 1;
    x &= 1;
    pendetect = x;

    x = __raw_readl(delta_base);
    x >>= 20;
    x ^= 1;
    x &= 1;
    cfdetect = x;

    x = __raw_readl(delta_base);
    x >>= 7;
    x &= 1;
    mmcdetect = x;

    init_waitqueue_head(&irex_er0100_button_waitq);
    button_pressed = 0xff;

    // INIT irex_er0100_button_waitq

    set_irq_type(BUTTON_ROW1_IRQ, IRQT_BOTHEDGE);
    if(request_irq(BUTTON_ROW1_IRQ, buttonpress1_irq_handler, SA_INTERRUPT, "Button press row 1", NULL)<0)
    {
        printk( KERN_ERR "Request for IRQ Button press row 1 (%u) failed\n", BUTTON_ROW1_IRQ);
        return -1;
    }
    set_irq_type(BUTTON_ROW2_IRQ, IRQT_BOTHEDGE);
    if(request_irq(BUTTON_ROW2_IRQ, buttonpress2_irq_handler, SA_INTERRUPT, "Button press row 2", NULL)<0)
    {
        printk( KERN_ERR "Request for IRQ Button press row 2 (%u) failed\n", BUTTON_ROW2_IRQ);
        return -1;
    }
    set_irq_type(BUTTON_ROW3_IRQ, IRQT_BOTHEDGE);
    if(request_irq(BUTTON_ROW3_IRQ, buttonpress3_irq_handler, SA_INTERRUPT, "Button press row 3", NULL)<0)
    {
        printk( KERN_ERR "Request for IRQ Button press row 3 (%u) failed\n", BUTTON_ROW3_IRQ);
        return -1;
    }
    set_irq_type(BUTTON_ROW4_IRQ, IRQT_BOTHEDGE);
    if(request_irq(BUTTON_ROW4_IRQ, buttonpress4_irq_handler, SA_INTERRUPT, "Button press row 4", NULL)<0)
    {
        printk( KERN_ERR "Request for IRQ Button press row 4 (%u) failed\n", BUTTON_ROW4_IRQ);
        return -1;
    }
    set_irq_type(POWER_IRQ, IRQT_BOTHEDGE);
    if(request_irq(POWER_IRQ, power_irq_handler, SA_INTERRUPT, "Power button", NULL)<0)
    {
        printk( KERN_ERR "Request for IRQ Power button (%u) failed\n", POWER_IRQ);
        return -1;
    }
    set_irq_type(PEN_DETECT_IRQ, IRQT_BOTHEDGE);
    if(request_irq(PEN_DETECT_IRQ, pen_detect_irq_handler, SA_INTERRUPT, "Pen detect", NULL)<0)
    {
        printk( KERN_ERR "Request for IRQ Pen detect (%u) failed\n", PEN_DETECT_IRQ);
        return -1;
    }
    set_irq_type(CF_CARDDETECT_IRQ, IRQT_BOTHEDGE);
    if(request_irq(CF_CARDDETECT_IRQ, cf_carddetect_irq_handler, SA_INTERRUPT, "cf detect", NULL)<0)
    {
        printk( KERN_ERR "Request for IRQ cf detect (%u) failed\n", CF_CARDDETECT_IRQ);
        return -1;
    }
    set_irq_type(MMC_CARDDETECT_IRQ, IRQT_BOTHEDGE);
    if(request_irq(MMC_CARDDETECT_IRQ, mmc_carddetect_irq_handler, SA_INTERRUPT, "mmc detect", NULL)<0)
    {
        printk( KERN_ERR "Request for IRQ mmc detect (%u) failed\n", MMC_CARDDETECT_IRQ);
        return -1;
    }

    last_button_pressed = jiffies;

    printk("Kernel: Init WACOM On\n");

    x = __raw_readl(delta_base + 0x2c);
    __raw_writel(x | 0x800, delta_base + 0x2c);

    disable_irq(PEN_DETECT_IRQ);
	return 0;
}

static int __devexit buttons_remove(struct platform_device *dev)
{
    int ret = 0;

    printk("buttons_remove\n");

    ret = unregister_chrdev(major, "buttons");
    if (ret < 0)
        printk("Error unregistering device: %d\n", ret); 

    disable_irq(BUTTON_ROW1_IRQ);
    disable_irq(BUTTON_ROW2_IRQ);
    disable_irq(BUTTON_ROW3_IRQ);
    disable_irq(BUTTON_ROW4_IRQ);

    disable_irq(POWER_IRQ);
    disable_irq(PEN_DETECT_IRQ);

    set_irq_type(BUTTON_ROW1_IRQ, IRQT_BOTHEDGE);
    set_irq_type(BUTTON_ROW2_IRQ, IRQT_BOTHEDGE);
    set_irq_type(BUTTON_ROW3_IRQ, IRQT_BOTHEDGE);
    set_irq_type(BUTTON_ROW4_IRQ, IRQT_BOTHEDGE);

    free_irq(BUTTON_ROW1_IRQ, NULL);
    free_irq(BUTTON_ROW2_IRQ, NULL);
    free_irq(BUTTON_ROW3_IRQ, NULL);
    free_irq(BUTTON_ROW4_IRQ, NULL);

    free_irq(POWER_IRQ, NULL);
    free_irq(PEN_DETECT_IRQ, NULL);
    free_irq(CF_CARDDETECT_IRQ, NULL);
    free_irq(MMC_CARDDETECT_IRQ, NULL);

	return ret;
}

static struct platform_device *buttons_platform_device;

static struct platform_driver buttons_device_driver = {
	.probe		= buttons_probe,
	.remove		= __devexit_p(buttons_remove),
	.driver		= {
		.name	= "buttons",
		.owner	= THIS_MODULE,
	},
};

static int __init buttons_init(void)
{
	int retval;

    printk("buttons_init\n");

	buttons_platform_device = platform_device_alloc("buttons", -1);
	if (!buttons_platform_device)
		return -ENOMEM;

	retval = platform_device_add(buttons_platform_device);
	if (retval < 0) {
		platform_device_put(buttons_platform_device);
		return retval;
	}

	retval = platform_driver_register(&buttons_device_driver);
	if (retval < 0)
		platform_device_unregister(buttons_platform_device);

	return retval;
}

static void __exit buttons_exit(void)
{
    printk("buttons_exit\n");

	platform_driver_unregister(&buttons_device_driver);

	platform_device_unregister(buttons_platform_device);
}

module_init(buttons_init);
module_exit(buttons_exit);

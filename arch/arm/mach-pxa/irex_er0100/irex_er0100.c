/*
 * Hardware definitions for iRex ER0100
 *
 * Use consistent with the GNU GPL is permitted,
 * provided that this copyright notice is
 * preserved in its entirety in all copies and derived works.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/input.h>
#include <linux/gpio_keys.h>
#include <linux/touchscreen-adc.h>
#include <linux/delay.h>

#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <asm/setup.h>

#include <mach/irqs.h>
#include <asm/mach/arch.h>

#include <mach/bitfield.h>
#include <mach/pxa2xx-regs.h>
#include <mach/regs-uart.h>
#include <mach/ohci.h>

#include <asm/mach/map.h>
#include <mach/irex_er0100.h>

#include "../generic.h"


static struct resource smc91x_resources[] = {
	[0] = {
		.name	= "smc91x-regs",
		.start	= (ETH_PHYS + 0x300),
		.end	= (ETH_PHYS + 0xfffff),
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= ETH_IRQ,
		.end	= ETH_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
/*
	[2] = {
		.name	= "smc91x-attrib",
		.start	= (ETH_PHYS + 0x02000000),
		.end	= (ETH_PHYS + 0x02000000 + 0xfffff),
		.flags	= IORESOURCE_MEM,
	},
*/
};

static struct platform_device smc91x_device = {
	.name		= "smc91x",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(smc91x_resources),
	.resource	= smc91x_resources,
};

static struct platform_device audio_device = {
    .name       = "pxa2xx-ac97",
    .id     = -1,
};

static struct platform_device i2c_device = {
	.name		= "pxa2xx-i2c",
	.id		= 0,
};

//GPCR(ETHERNET_ON_GPIO) |= GPIO_bit(ETHERNET_ON_GPIO);
//udelay(1500);

static struct platform_device *devices[] __initdata = {
	&smc91x_device,
	&audio_device,
//    &i2c_device,
    
};

static struct map_desc irex_er0100_io_desc[] __initdata = {
	{	/* LAN91C111 IO */
		.virtual	= ETH_VIRT,
		.pfn		= __phys_to_pfn(ETH_PHYS),
		.length		= ETH_SIZE,
		.type		= MT_DEVICE
	}, {	/* LAN91C111 Attr */
		.virtual	= (ETH_VIRT+0x00100000),
		.pfn		= __phys_to_pfn(ETH_PHYS+0x02000000),
		.length		= ETH_SIZE,
		.type		= MT_DEVICE
	}, {	/* Delta ASIC */
		.virtual	=               0xF8E00000,
		.pfn		= __phys_to_pfn(0x40e00000),
		.length		= 0x100000,
		.type		= MT_DEVICE
	}
};

static void __init irex_er0100_map_io(void)
{
	pxa_map_io();
    iotable_init(irex_er0100_io_desc, ARRAY_SIZE(irex_er0100_io_desc));


    /* This is for the SMC chip select */
    //set_GPIO_mode(GPIO79_nCS_3_MD);
    pxa_gpio_mode(GPIO79_nCS_3_MD);

    /* setup sleep mode values */
    PWER  = 0x80000002;
    PFER  = 0x00000000;
    PRER  = 0x00000002;
    PGSR0 = 0x00008000;
    PGSR0 = 0x00008040;
    PGSR1 = 0x003F0202;
    PGSR2 = 0x0001C000;
    PCFR |= PCFR_OPDE;

    /* DOCFlash address bit 0 is connected to GPIO10 and must be set to 0 */
    GPDR(GPIO_bit(10)) |= GPIO_bit(10);
    GPCR(GPIO_bit(10)) |= GPIO_bit(10);
    GPDR(GPIO_bit(7)) |= GPIO_bit(7);
    GPSR(GPIO_bit(7)) |= GPIO_bit(7);
}

void irex_led(void)
{
    GPSR(ACTIVITY_LED_GPIO) = GPIO_bit(ACTIVITY_LED_GPIO);
}

EXPORT_SYMBOL(irex_led);

static void __init irex_er0100_init(void)
{
	CKEN &= 0x0000DCF4;
    //pxa_set_cken(0x0000DCF4, 1);


/*
//?
// ETH_RESET_GPIO 
    printk("Resetting WLAN.\n");
    GPSR(9) |= GPIO_bit(9);
    mdelay(100);
    GPCR(9) |= GPIO_bit(9);
    mdelay(100);
*/

// SMC
// Off
//    GPSR(ETHERNET_ON_GPIO) |= GPIO_bit(ETHERNET_ON_GPIO);
//   udelay(1500);
// On
    GPCR(ETHERNET_ON_GPIO) |= GPIO_bit(ETHERNET_ON_GPIO);
    GPSR(CF_ON_GPIO) |= GPIO_bit(CF_ON_GPIO);
    udelay(1500);
//
//	set_pxa_fb_info(&sony_acx526akm);

//	platform_device_register(&htcuniversal_asic3);
	platform_add_devices(devices, ARRAY_SIZE(devices) );
//	pxa_set_ficp_info(&htcuniversal_ficp_platform_data);
//	pxa_set_ohci_info(&htcuniversal_ohci_platform_data);

//	led_trigger_register_shared("htcuniversal-radio", &htcuniversal_radio_trig);
    //irex_led();
}

static void __init irex_er0100_init_irq(void)
{
    pxa_init_irq();
//MV SMC?
    //set_GPIO_IRQ_edge(ETH_GPIO,GPIO_RISING_EDGE);
    //set_irq_type(gpio_to_irq(ETH_GPIO), IRQT_RISING)
    set_irq_type(ETH_IRQ, IRQT_RISING);
}

static void __init fixup_irex_er0100(struct machine_desc *desc,
        struct tag *tags, char **cmdline, struct meminfo *mi)
{
    mi->nr_banks=1;
    mi->bank[0].start = 0xa0000000;
    mi->bank[0].node = 0;
    mi->bank[0].size = (64*1024*1024);
}

MACHINE_START(PXA_IREX_ER0100, "iRex Technologies ER0100 eReader")
	/* Maintainer A.Lown */
	.phys_io	= 0x40000000,
	.io_pg_offst	= (io_p2v(0x40000000) >> 18) & 0xfffc,
//	.boot_params	= 0xa0000100,
        .fixup      = fixup_irex_er0100,
	.map_io		= irex_er0100_map_io,
	.init_irq	= irex_er0100_init_irq,
	.init_machine	= irex_er0100_init,
	.timer		= &pxa_timer,
MACHINE_END

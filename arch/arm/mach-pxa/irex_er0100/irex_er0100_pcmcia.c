/*
 *  linux/drivers/pcmcia/pxa/irex_er0100_pcmcia.c
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

#include <asm/mach-types.h>
#include <asm/hardware.h>
#include <asm/irq.h>

#include <asm/arch/pxa-regs.h>
#include <asm/arch/hx4700-gpio.h>
#include <asm/arch/hx4700-asic.h>
#include <asm/arch/hx4700-core.h>
#include <asm/hardware/ipaq-asic3.h>
#include <linux/mfd/asic3_base.h>
#include <asm/arch/irex_er0100.h>

#include "irex_er0100_pcmcia.h"
#include "../../../../drivers/pcmcia/soc_common.h"

static struct pcmcia_irqs irex_er0100_cd_irqs[] = {
        { 0, S0_CD_IRQ,  "PCMCIA (0) CD" },
        { 1, S1_CD_IRQ,  "PCMCIA (1) CD" },
        { 0, S0_BVD_IRQ,  "PCMCIA (0) BVD" },
        { 1, S1_BVD_IRQ,  "PCMCIA (1) BVD" },
};

static int irex_er0100_pcmcia_hw_init(struct soc_pcmcia_socket *skt)
{
/*
	set_GPIO_IRQ_edge(S0_CD_GPIO,GPIO_BOTH_EDGES);
	set_GPIO_IRQ_edge(S0_READY_GPIO,GPIO_FALLING_EDGE);
	set_GPIO_IRQ_edge(S0_BVD_GPIO,GPIO_FALLING_EDGE);
	set_GPIO_IRQ_edge(S1_CD_GPIO,GPIO_BOTH_EDGES);
	set_GPIO_IRQ_edge(S1_READY_GPIO,GPIO_FALLING_EDGE);
	set_GPIO_IRQ_edge(S1_BVD_GPIO,GPIO_FALLING_EDGE);


	if(request_irq(S0_CD_IRQ, init->handler, SA_INTERRUPT,"PCMCIA (0) CD", NULL)<0)
	{
		printk( KERN_ERR __FUNCTION__ ": Request for IRQ S0_CD (%u) failed\n", S0_CD_IRQ);
		return -1;
	}

	if(request_irq(S0_BVD_IRQ, init->handler, SA_INTERRUPT,"PCMCIA (0) BVD", NULL)<0)
	{
		printk( KERN_ERR __FUNCTION__ ": Request for IRQ S0_BVD (%u) failed\n", S0_BVD_IRQ);
		return -1;
	}
	if(request_irq(S1_CD_IRQ, init->handler, SA_INTERRUPT,"PCMCIA (1) CD", NULL)<0)
	{
		printk( KERN_ERR __FUNCTION__ ": Request for IRQ S1_CD (%u) failed\n", S1_CD_IRQ);
		return -1;
	}

	if(request_irq(S1_BVD_IRQ, init->handler, SA_INTERRUPT,"PCMCIA (1) BVD", NULL)<0)
	{
		printk( KERN_ERR __FUNCTION__ ": Request for IRQ S1_BVD (%u) failed\n", S1_BVD_IRQ);
		return -1;
	}
*/

    if(skt->nr == 0) {
        set_irq_type(gpio_to_irq(S0_CD_GPIO), IRQT_BOTHEDGE);
        set_irq_type(gpio_to_irq(S0_READY_GPIO), IRQT_FALLING);
        set_irq_type(gpio_to_irq(S0_BVD_GPIO), IRQT_FALLING);
    } else {
        set_irq_type(gpio_to_irq(S1_CD_GPIO), IRQT_BOTHEDGE);
        set_irq_type(gpio_to_irq(S1_READY_GPIO), IRQT_FALLING);
        set_irq_type(gpio_to_irq(S1_BVD_GPIO), IRQT_FALLING);
    }
    skt->irq = (skt->nr == 0) ? S0_READY_IRQ : S1_READY_IRQ;
    return soc_pcmcia_request_irqs(skt, irex_er0100_cd_irqs, ARRAY_SIZE(irex_er0100_cd_irqs));
}


static void irex_er0100_pcmcia_hw_shutdown(struct soc_pcmcia_socket *skt)
{
/*
	free_irq(S0_CD_IRQ, NULL);
	free_irq(S0_BVD_IRQ, NULL);
	free_irq(S1_CD_IRQ, NULL);
	free_irq(S1_BVD_IRQ, NULL);
*/

    soc_pcmcia_free_irqs( skt, irex_er0100_cd_irqs, ARRAY_SIZE(irex_er0100_cd_irqs) );
}

static void irex_er0100_pcmcia_socket_state(struct soc_pcmcia_socket *skt,  struct pcmcia_state *state)
{
/*
	int sock;

	if (state_array->size < 2) {
		printk(__FUNCTION__ ": Size too small\n");
		return -1;
	}
	
	memset(state_array->state, 0, (state_array->size)*sizeof(struct pcmcia_state));
*/

    if(skt->nr == 0) {
        state->detect= (GPLR(S0_CD_GPIO) & GPIO_bit(S0_CD_GPIO))?0:1;
        state->ready = (GPLR(S0_READY_GPIO) & GPIO_bit(S0_READY_GPIO))?1:0;
    } else {
        state->detect= (GPLR(S1_CD_GPIO) & GPIO_bit(S1_CD_GPIO))?0:1;
        state->ready = (GPLR(S1_READY_GPIO) & GPIO_bit(S1_READY_GPIO))?1:0;
    }
	state->bvd1 = 1;
	state->bvd2 = 1;
	state->wrprot = 0;
	state->vs_3v = 1;
	state->vs_Xv = 0;

/*
	state[1].detect= (GPLR(S1_CD_GPIO) & GPIO_bit(S1_CD_GPIO))?0:1;
	state[1].ready = (GPLR(S1_READY_GPIO) & GPIO_bit(S1_READY_GPIO))?1:0;
	state[1].bvd1 = 1;
	state[1].bvd2 = 1;
	state[1].wrprot = 0;
	state[1].vs_3v = 1;
	state[1].vs_Xv = 0;
	return 1;
*/
}

/*
static int irex_er0100_pcmcia_get_irq_info(struct pcmcia_irq_info *info)
{
	switch(info->sock)
	{
		case 0:
			info->irq=S0_READY_IRQ;
			break;
		case 1:
			info->irq=S1_READY_IRQ;
			break;
		default:
			return -1;
	}
	return 0;
}
*/

spinlock_t pcmcia_lock;

static int irex_er0100_pcmcia_configure_socket(struct soc_pcmcia_socket *skt, const socket_state_t *state)
{
/*
	if(sock>2)
		return -1;
*/
	return 0;
}

/*
 *  * Enable card status IRQs on (re-)initialisation.  This can
 *   * be called at initialisation, power management event, or
 *    * pcmcia event.
 *     */
static void
irex_er0100_pcmcia_socket_init(struct soc_pcmcia_socket *skt)
{
//       _debug_func ("\n");
}

/*
 *  * Disable card status IRQs on suspend.
 *   */
static void
irex_er0100_pcmcia_socket_suspend( struct soc_pcmcia_socket *skt )
{
//        _debug_func ("\n");
}

static struct pcmcia_low_level irex_er0100_pcmcia_ops = {
        .owner                  = THIS_MODULE,
        .nr                     = 2,
        .hw_init                = irex_er0100_pcmcia_hw_init,
        .hw_shutdown            = irex_er0100_pcmcia_hw_shutdown,
        .socket_state           = irex_er0100_pcmcia_socket_state,
        //irex_er0100_pcmcia_get_irq_info,
        .configure_socket       = irex_er0100_pcmcia_configure_socket,
        .socket_init            = irex_er0100_pcmcia_socket_init,
        .socket_suspend         = irex_er0100_pcmcia_socket_suspend,
};

static struct platform_device irex_er0100_pcmcia_device = {
        .name                   = "pxa2xx-pcmcia",
        .dev                    = {
                .platform_data  = &irex_er0100_pcmcia_ops,
        }
};

static int __init
irex_er0100_pcmcia_init(void)
{
/*
        if (!machine_is_h4700())
                return -ENODEV;
        _debug_func ("\n");
*/

        return platform_device_register( &irex_er0100_pcmcia_device );
}

static void __exit
irex_er0100_pcmcia_exit(void)
{
        platform_device_unregister( &irex_er0100_pcmcia_device );
}

module_init(irex_er0100_pcmcia_init);
module_exit(irex_er0100_pcmcia_exit);

MODULE_AUTHOR("MV");
MODULE_DESCRIPTION("iRex ER0100 PCMCIA/CF platform-specific driver");
MODULE_LICENSE("GPL");

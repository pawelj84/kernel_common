#include <mach/irqs.h>

#define GPIO_2_80_TO_IRQ(x) \
            PXA_IRQ((x) - 2 + 32)

/*
 *  linux/include/asm-arm/arch-pxa/irex_er0100.h
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

/* SD/MMC card IF */
#define MMC_SD_CLK		6
#define MMC_SD_CS		8
#define MMC_SD_CD		7
#define MMC_SD_WP		73

/* Power supply enable bits */
#define ETHERNET_ON_GPIO	14
#define SHUTDOWN_GPIO		27
#define WACOM_ON_GPIO		75
#define CF_ON_GPIO		19
#define MMC_ON_GPIO		33
#define CF_CARDDETECT_GPIO	20

/* PCMCIA */
//#ifdef CONFIG_PCMCIA
#define S0_BVD_GPIO	77
#define S0_BVD_IRQ	GPIO_2_80_TO_IRQ(S0_BVD_GPIO)
#define S1_BVD_GPIO	21
#define S1_BVD_IRQ	GPIO_2_80_TO_IRQ(S1_BVD_GPIO)
#define S0_READY_GPIO	44
#define S0_READY_IRQ	GPIO_2_80_TO_IRQ(S0_READY_GPIO)
#define S1_READY_GPIO	45
#define S1_READY_IRQ	GPIO_2_80_TO_IRQ(S1_READY_GPIO)
#define S0_CD_GPIO	43
#define S0_CD_IRQ	GPIO_2_80_TO_IRQ(S0_CD_GPIO)
#define S1_CD_GPIO	42
#define S1_CD_IRQ	GPIO_2_80_TO_IRQ(S1_CD_GPIO)
//#endif

/* Ethernet */
//#if defined(CONFIG_SMC91X) | defined(CONFIG_SMC91X_MODULE)
#define ETH_GPIO	13
#define ETH_RESET_GPIO	9
#define ETH_IRQ		GPIO_2_80_TO_IRQ(ETH_GPIO)
#define ETH_PHYS	0x0c000000
#define ETH_SIZE	0x00100000
#define ETH_VIRT	0xf1000000
//#endif

/* UCB1400 */
//#if defined(CONFIG_MCP_UCB1400_TS) | defined(CONFIG_MCP_UCB1400_TS_MODULE)
#define UCB1400_GPIO	32
#define UCB1400_IRQ	GPIO_2_80_TO_IRQ(UCB1400_GPIO)
//#endif
//#if defined(CONFIG_SOUND_PXA_AC97) | defined (CONFIG_SOUND_PXA_AC97_MODULE)
#define UCB1400_MUTE    (1<<0)
//#endif

/* Buttons */
//#if defined(CONFIG_MACH_PXA_IREX_ER0100)
#define BUTTON_COLUMN1_GPIO		38
#define BUTTON_COLUMN2_GPIO		37
#define BUTTON_COLUMN3_GPIO		36
#define BUTTON_COLUMN4_GPIO		35

#define BUTTON_ROW1_GPIO		2
#define BUTTON_ROW2_GPIO		3
#define BUTTON_ROW3_GPIO		4
#define BUTTON_ROW4_GPIO		5

#define PEN_DETECT_GPIO			1	
#define POWER_GPIO		26

#define BUTTON_ROW1_IRQ		GPIO_2_80_TO_IRQ(BUTTON_ROW1_GPIO)
#define BUTTON_ROW2_IRQ		GPIO_2_80_TO_IRQ(BUTTON_ROW2_GPIO)
#define BUTTON_ROW3_IRQ		GPIO_2_80_TO_IRQ(BUTTON_ROW3_GPIO)
#define BUTTON_ROW4_IRQ		GPIO_2_80_TO_IRQ(BUTTON_ROW4_GPIO)

/* Valentin*/
#define PEN_DETECT_IRQ		IRQ_GPIO1
#define POWER_IRQ		GPIO_2_80_TO_IRQ(POWER_GPIO)
#define CF_CARDDETECT_IRQ	GPIO_2_80_TO_IRQ(CF_CARDDETECT_GPIO)
#define MMC_CARDDETECT_IRQ	GPIO_2_80_TO_IRQ(MMC_SD_CD)

#define ACTIVITY_LED_GPIO	23

#define BATTERY_GPIO		25
#define BATTERY_IRQ		GPIO_2_80_TO_IRQ(BATTERY_GPIO)
//#endif

/* E-Ink */
#ifdef CONFIG_FB_EINK
#define EINK_DATA0        58
#define EINK_DATA1        59
#define EINK_DATA2        60
#define EINK_DATA3        61
#define EINK_DATA4        62
#define EINK_DATA5        63
#define EINK_DATA6        64
#define EINK_DATA7        65
#define EINK_RNW          66
#define EINK_NDS1         67
#define EINK_CND          68
#define EINK_WUP          69
#define EINK_NACK         70
#endif


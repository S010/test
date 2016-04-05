/*
 * Defintions for Freescale i.MX6 Dual/Quad SoC.
 */

#ifndef __IMX6DQ_H__
#define __IMX6DQ_H__

#include <stdint.h>
#include "bitops.h"

#define IOMUXC_SW_MUX_CTL_PAD_GPIO00		(*((volatile uint32_t *)0x20e0220u))
#define IOMUXC_SW_MUX_CTL_PAD_GPIO01		(*((volatile uint32_t *)0x20e0224u))
#define IOMUXC_SW_MUX_CTL_PAD_GPIO02		(*((volatile uint32_t *)0x20e0234u))
#define IOMUXC_SW_MUX_CTL_PAD_GPIO03		(*((volatile uint32_t *)0x20e022cu))
#define IOMUXC_SW_MUX_CTL_PAD_GPIO04		(*((volatile uint32_t *)0x20e0238u))
#define IOMUXC_SW_MUX_CTL_PAD_GPIO05		(*((volatile uint32_t *)0x20e023cu))
#define IOMUXC_SW_MUX_CTL_PAD_GPIO06		(*((volatile uint32_t *)0x20e0230u))
#define IOMUXC_SW_MUX_CTL_PAD_GPIO07		(*((volatile uint32_t *)0x20e0240u))
#define IOMUXC_SW_MUX_CTL_PAD_GPIO08		(*((volatile uint32_t *)0x20e0244u))
#define IOMUXC_SW_MUX_CTL_PAD_GPIO09		(*((volatile uint32_t *)0x20e0228u))
#define IOMUXC_SW_MUX_CTL_PAD_GPIO16		(*((volatile uint32_t *)0x20e0248u))
#define IOMUXC_SW_MUX_CTL_PAD_GPIO17		(*((volatile uint32_t *)0x20e024cu))
#define IOMUXC_SW_MUX_CTL_PAD_GPIO18		(*((volatile uint32_t *)0x20e0250u))
#define IOMUXC_SW_MUX_CTL_PAD_GPIO19		(*((volatile uint32_t *)0x20e0254u))
#define IOMUXC_SW_MUX_CTL_PAD_EIM_DATA26	(*((volatile uint32_t *)0x20e00bcu))
#define IOMUXC_SW_MUX_CTL_PAD_SD3_RESET		(*((volatile uint32_t *)0x20e02d0u))

/* MUX Mode Select */
#define MUX_MODE_HI	2
#define MUX_MODE_LO	0
#define MUX_MODE_MASK	BITMASK(MUX_MODE_HI, MUX_MODE_LO)
#define MUX_MODE_ALT0	(0 << MUX_MODE_LO)
#define MUX_MODE_ALT1	(1 << MUX_MODE_LO)
#define MUX_MODE_ALT2	(2 << MUX_MODE_LO)
#define MUX_MODE_ALT3	(3 << MUX_MODE_LO)
#define MUX_MODE_ALT4	(4 << MUX_MODE_LO)
#define MUX_MODE_ALT5	(5 << MUX_MODE_LO)

/* Software Input On (force input path). */
#define SION_HI		4
#define SION_LO		4
#define SION_MASK	BITMASK(SION_HI, SION_LO)
#define SION_ENABLED	(1 << SION_LO)
#define SION_DISABLED	(0 << SION_LO)



#define IOMUXC_SW_PAD_CTL_PAD_GPIO18		(*((volatile uint32_t *)0x20e0620u))
#define IOMUXC_SW_PAD_CTL_PAD_GPIO19		(*((volatile uint32_t *)0x20e0624u))
#define IOMUXC_SW_PAD_CTL_PAD_EIM_DATA26	(*((volatile uint32_t *)0x20e03d0u))
#define IOMUXC_SW_PAD_CTL_PAD_SD3_RESET		(*((volatile uint32_t *)0x20e06b8u))

/* Hysteresis Enable */
#define HYS_HI		16
#define HYS_LO		16
#define HYS_MASK	BITMASK(HYS_HI, HYS_LO)
#define HYS_DISABLED	(0 << HYS_LO)
#define HYS_ENABLED	(1 << HYS_LO)

/* Pull Up / Down Configuration */
#define PUS_HI		15
#define PUS_LO		14
#define PUS_MASK	BITMASK(PUS_HI, PUS_LO)
#define PUS_100K_OHM_PD	(0 << PUS_LO)
#define PUS_47K_OHM_PU	(1 << PUS_LO)
#define PUS_100K_OHM_PU	(2 << PUS_LO)
#define PUS_22K_OHM_PU	(3 << PUS_LO)

/* Pull / Keeper Select */
#define PUE_HI		13
#define PUE_LO		13
#define PUE_MASK	BITMASK(PUS_HI, PUS_LO)
#define PUE_KEEP	(0 << PUE_LO)
#define PUE_PULL	(1 << PUE_LO)

/* Pull / Keep Enable */
#define PKE_HI		12
#define PKE_LO		12
#define PKE_MASK	BITMASK(PKE_HI, PKE_LO)
#define PKE_DISABLED	(0 << PKE_LO)
#define PKE_ENABLED	(1 << PKE_LO)

/* Open Drain enable */
#define ODE_HI		11
#define ODE_LO		11
#define ODE_MASK	BITMASK(ODE_HI, ODE_LO)
#define ODE_DISABLED	(0 << ODE_LO)
#define ODE_ENABLED	(1 << ODE_LO)

/* Speed (influences operational frequency) */
#define SPEED_HI	7
#define SPEED_LO	6
#define SPEED_MASK	BITMASK(SPEED_HI, SPEED_LO)
#define SPEED_LOW	(0 << SPEED_LO)
#define SPEED_MEDIUM	(1 << SPEED_LO)
#define SPEED_MEDIUM_ALT	(2 << SPEED_LO)
#define SPEED_MAXIMUM	(3 << SPEED_LO)

/* Drive Strength */
#define DSE_HI		5
#define DSE_LO		3
#define DSE_MASK	BITMASK(DSE_HI, DSE_LO)
#define DSE_HIZ		(0 << DSE_LO)
#define DSE_260_OHM	(1 << DSE_LO)
#define DSE_130_OHM	(2 << DSE_LO)
#define DSE_90_OHM	(3 << DSE_LO)
#define DSE_60_OHM	(4 << DSE_LO)
#define DSE_50_OHM	(5 << DSE_LO)
#define DSE_40_OHM	(6 << DSE_LO)
#define DSE_33_OHM	(7 << DSE_LO)

/* Slew Rate (influences operational frequency) */
#define SRE_HI		0
#define SRE_LO		0
#define SRE_MASK	BITMASK(SRE_HI, SRE_LO)
#define SRE_SLOW	(0 << SRE_LO)
#define SRE_FAST	(1 << SRE_LO)



#define GPIO1_DR	(*((volatile uint32_t *)0x209c000u))
#define GPIO1_GDIR	(*((volatile uint32_t *)0x209c004u))
#define GPIO1_PSR	(*((volatile uint32_t *)0x209c008u))
#define GPIO1_ICR1	(*((volatile uint32_t *)0x209c00cu))
#define GPIO1_ICR2	(*((volatile uint32_t *)0x209c010u))
#define GPIO1_IMR	(*((volatile uint32_t *)0x209c014u))
#define GPIO1_ISR	(*((volatile uint32_t *)0x209c018u))
#define GPIO1_EDGE_SEL	(*((volatile uint32_t *)0x209c01cu))

#define GPIO2_DR	(*((volatile uint32_t *)0x20a0000u))
#define GPIO2_GDIR	(*((volatile uint32_t *)0x20a0004u))
#define GPIO2_PSR	(*((volatile uint32_t *)0x20a0008u))
#define GPIO2_ICR1	(*((volatile uint32_t *)0x20a000cu))
#define GPIO2_ICR2	(*((volatile uint32_t *)0x20a0010u))
#define GPIO2_IMR	(*((volatile uint32_t *)0x20a0014u))
#define GPIO2_ISR	(*((volatile uint32_t *)0x20a0018u))
#define GPIO2_EDGE_SEL	(*((volatile uint32_t *)0x20a001cu))

#define GPIO3_DR	(*((volatile uint32_t *)0x20a4000u))
#define GPIO3_GDIR	(*((volatile uint32_t *)0x20a4004u))
#define GPIO3_PSR	(*((volatile uint32_t *)0x20a4008u))
#define GPIO3_ICR1	(*((volatile uint32_t *)0x20a400cu))
#define GPIO3_ICR2	(*((volatile uint32_t *)0x20a4010u))
#define GPIO3_IMR	(*((volatile uint32_t *)0x20a4014u))
#define GPIO3_ISR	(*((volatile uint32_t *)0x20a4018u))
#define GPIO3_EDGE_SEL	(*((volatile uint32_t *)0x20a401cu))

#define GPIO4_DR	(*((volatile uint32_t *)0x20a8000u))
#define GPIO4_GDIR	(*((volatile uint32_t *)0x20a8004u))
#define GPIO4_PSR	(*((volatile uint32_t *)0x20a8008u))
#define GPIO4_ICR1	(*((volatile uint32_t *)0x20a800cu))
#define GPIO4_ICR2	(*((volatile uint32_t *)0x20a8010u))
#define GPIO4_IMR	(*((volatile uint32_t *)0x20a8014u))
#define GPIO4_ISR	(*((volatile uint32_t *)0x20a8018u))
#define GPIO4_EDGE_SEL	(*((volatile uint32_t *)0x20a801cu))

#define GPIO5_DR	(*((volatile uint32_t *)0x20ac000u))
#define GPIO5_GDIR	(*((volatile uint32_t *)0x20ac004u))
#define GPIO5_PSR	(*((volatile uint32_t *)0x20ac008u))
#define GPIO5_ICR1	(*((volatile uint32_t *)0x20ac00cu))
#define GPIO5_ICR2	(*((volatile uint32_t *)0x20ac010u))
#define GPIO5_IMR	(*((volatile uint32_t *)0x20ac014u))
#define GPIO5_ISR	(*((volatile uint32_t *)0x20ac018u))
#define GPIO5_EDGE_SEL	(*((volatile uint32_t *)0x20ac01cu))

#define GPIO6_DR	(*((volatile uint32_t *)0x20b0000u))
#define GPIO6_GDIR	(*((volatile uint32_t *)0x20b0004u))
#define GPIO6_PSR	(*((volatile uint32_t *)0x20b0008u))
#define GPIO6_ICR1	(*((volatile uint32_t *)0x20b000cu))
#define GPIO6_ICR2	(*((volatile uint32_t *)0x20b0010u))
#define GPIO6_IMR	(*((volatile uint32_t *)0x20b0014u))
#define GPIO6_ISR	(*((volatile uint32_t *)0x20b0018u))
#define GPIO6_EDGE_SEL	(*((volatile uint32_t *)0x20b001cu))

#define GPIO7_DR	(*((volatile uint32_t *)0x20b4000u))
#define GPIO7_GDIR	(*((volatile uint32_t *)0x20b4004u))
#define GPIO7_PSR	(*((volatile uint32_t *)0x20b4008u))
#define GPIO7_ICR1	(*((volatile uint32_t *)0x20b400cu))
#define GPIO7_ICR2	(*((volatile uint32_t *)0x20b4010u))
#define GPIO7_IMR	(*((volatile uint32_t *)0x20b4014u))
#define GPIO7_ISR	(*((volatile uint32_t *)0x20b4018u))
#define GPIO7_EDGE_SEL	(*((volatile uint32_t *)0x20b401cu))

#define GPIO_INPUT	0
#define GPIO_OUTPUT	1

#endif

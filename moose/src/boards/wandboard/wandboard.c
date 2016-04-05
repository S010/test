#include <stdbool.h>
#include <error.h>
#include <gpio.h>
#include <imx6dq.h>

static void
gpio_init_pad(volatile uint32_t *regp)
{
	uint32_t	 reg;

	reg = *regp;
	REG_SET(reg, PKE, DISABLED);
	REG_SET(reg, ODE, DISABLED);
	REG_SET(reg, DSE, 90_OHM);
	*regp = reg;
}

static void
gpio_init_mux(volatile uint32_t *regp)
{
	uint32_t	reg;

	reg = *regp;
	REG_SET(reg, SION, DISABLED);
	REG_SET(reg, MUX_MODE, ALT5);
	*regp = reg;
}

int
gpio_init(void)
{
	gpio_init_mux(&IOMUXC_SW_MUX_CTL_PAD_SD3_RESET); /* GPIO7_IO08 */
	gpio_init_pad(&IOMUXC_SW_PAD_CTL_PAD_SD3_RESET);
	GPIO7_GDIR |= GPIO_OUTPUT << 8;

	gpio_init_mux(&IOMUXC_SW_MUX_CTL_PAD_EIM_DATA26); /* GPIO3_IO26 */
	gpio_init_pad(&IOMUXC_SW_PAD_CTL_PAD_EIM_DATA26);
	GPIO3_GDIR |= GPIO_OUTPUT << 26;

	gpio_init_mux(&IOMUXC_SW_MUX_CTL_PAD_GPIO18); /* GPIO7_IO13 */
	gpio_init_pad(&IOMUXC_SW_PAD_CTL_PAD_GPIO18);
	GPIO7_GDIR |= GPIO_OUTPUT << 13;

	gpio_init_mux(&IOMUXC_SW_MUX_CTL_PAD_GPIO19); /* GPIO4_IO05 */
	gpio_init_pad(&IOMUXC_SW_PAD_CTL_PAD_GPIO19);
	GPIO4_GDIR |= GPIO_OUTPUT << 5;

	return 0;
}

int
gpio_toggle(unsigned index, bool enable)
{
	(void)index;
	(void)enable;

	GPIO7_DR |= 1 << 8;
	GPIO3_DR |= 1 << 26;
	GPIO7_DR |= 1 << 13;
	GPIO4_DR |= 1 << 5; 

	return 0;
}

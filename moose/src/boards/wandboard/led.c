#include <stdint.h>
#include <stdbool.h>
#include <gpio.h>

static void
led(void)
{
	(void)gpio_init();
	gpio_toggle(0, true);
}

int
main(void)
{
	led();

	return 0;
}


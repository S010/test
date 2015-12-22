#ifndef __GPIO_H__
#define __GPIO_H__

#include <err.h>
#include <stdbool.h>

/*
 * Initialize GPIO subsystem.
 *
 * Other gpio_* functions can be used only after a successful call to
 * gpio_init().
 */
err_t gpio_init(void);

/*
 * Set or clear a GPIO pin.
 */
err_t gpio_toggle(int, bool);

#endif

#ifndef __GPIO_H__
#define __GPIO_H__

#include <error.h>
#include <stdbool.h>

int gpio_init(void);
int gpio_toggle(unsigned, bool);

#endif

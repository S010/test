#ifndef __GPIO_H__
#define __GPIO_H__

#include <error.h>
#include <types.h>

int gpio_init(void);
int gpio_toggle(unsigned, bool);

#endif

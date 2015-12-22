#ifndef BITOPS_H
#define BITOPS_H

#define BITMASK_N(n)	(~(~0u << (n)))
#define BITMASK(hi, lo)	(BITMASK_N(hi - lo + 1) << (lo))

#define _REG_SET(reg, mask, val)	do {				\
						(reg) &= ~mask;		\
						(reg) |= val;		\
					} while (0)
#define REG_SET(reg, name, val)	_REG_SET(reg, name ## _MASK, name ## _ ## val)

#endif

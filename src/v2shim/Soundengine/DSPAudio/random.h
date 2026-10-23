/* LXRXXL shim: the LXR's random numbers. The firmware reads the STM32's hardware RNG (random.c); src/v2shim/random.c
 * gives the same interface from a xorshift32 generator, per instance (compiled as an engine unit). */
#ifndef RANDOM_H_
#define RANDOM_H_
#include <stdint.h>
void initRng(void);
uint32_t GetRngValue(void);
#endif

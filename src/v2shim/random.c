/* LXRXXL: GetRngValue() for the LXR engine (the firmware's random.c reads the STM32's hardware RNG): a xorshift32
 * generator, full 32-bit words like the hardware's. An engine unit, so each instance keeps its own state. */
#include <stdint.h>
#include "Soundengine/DSPAudio/random.h"

static uint32_t rng = 0x9E3779B9u;

void initRng(void) { if (!rng) rng = 0x9E3779B9u; }

uint32_t GetRngValue(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return rng;
}

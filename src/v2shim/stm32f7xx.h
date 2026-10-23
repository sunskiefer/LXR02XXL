/* LXRXXL shim: what the LXR firmware takes from the STM32 HAL -- the integer types, the Cortex-M intrinsics as
 * portable C (cmsis_intrinsics.h), and nothing of the hardware. The few register reads left in the sound engine
 * (the mixer's jack switches) read the stand-ins in main.h. */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include "cmsis_intrinsics.h"

/* LXRXXL shim: of the firmware's main.h the sound engine only reads the output jack switches (mixer.c). The plugin
 * has one stereo out: every jack reads as plugged, so each routing keeps its side, and lxr_dsp.c adds OUT 1 and
 * OUT 2 into it (what the LXR-02 sounds like with both outs mixed). */
#pragma once
#include "stm32f7xx_hal.h"
#include "Soundengine/config.h"
typedef struct { uint32_t IDR; } LxrGpio;
extern LxrGpio lxr_jacks;
#define switch_l1_Pin 1u
#define switch_r1_Pin 1u
#define switch_l2_Pin 1u
#define switch_r2_Pin 1u
#define switch_l1_GPIO_Port (&lxr_jacks)
#define switch_r1_GPIO_Port (&lxr_jacks)
#define switch_l2_GPIO_Port (&lxr_jacks)
#define switch_r2_GPIO_Port (&lxr_jacks)

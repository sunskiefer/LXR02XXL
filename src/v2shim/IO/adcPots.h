/* LXRXXL shim: the LXR-02's six voice faders, as the firmware reads them (adcPots.c: position squared), set by the
 * plugin's faders (lxr_dsp.c). */
#pragma once
#include <stdint.h>
float adc_getFaderVolume(uint8_t idx);

/* LXRXXL -- the LXR-02 sound engine (the Erica Synths LXR firmware's own DSP, GPL-3.0) driven as a plugin.
 *
 * The voices, filters, envelopes, LFOs, modulation, transient generator, decimators, the mixer and the FX bus are the
 * firmware's code (third_party/lxr-desktop-app), parameters go in through the firmware's own handler (cc_handler.inc)
 * as on the machine. Everything here works on the engine's global state, so it must only run with this instance's
 * state swapped in (lxr_state.c). */
#pragma once
#include <stdint.h>

#define LXR_NUM_SOUND_PARAMS 228   /* the 0.37-compatible part of a kit: parameters 0..227 */

/* The rest of an LXR-02 kit, parameters 228..246 (Parameters.h): pulse width per voice, the FX unit and the morph
 * kit destination. A kit is these 247 bytes, as the LXR-02 saves it (NN-NAME.SND without its name). */
enum {
    EXT_PWM1 = 0, EXT_PWM2, EXT_PWM3, EXT_PWM4, EXT_PWM5, EXT_PWM6,
    EXT_FX_TYPE,       /* 0 off, 1 drv, 2 rng, 3 cmp, 4 del, 5 rev */
    EXT_FX_OUT,        /* 0 St1 .. 5 R2 (the output the FX signal is mixed to) */
    EXT_FX_DW,         /* dry/wet 0..100 */
    EXT_FX_DRVTYPE,    /* drive: 0 Tub, 1 Fld, 2 Clp */
    EXT_FX_P1, EXT_FX_P2, EXT_FX_P3, EXT_FX_P4,
    EXT_FX_RATIO,      /* compressor ratio: 1:1 2:1 3:1 4:1 6:1 8:1 */
    EXT_FX_RINGWAVE,   /* ring modulator waveform */
    EXT_FX_REVERBTYPE, /* reverb: 0 Plt */
    EXT_FX_DELAYTYPE,  /* 0 Del, 1 Pp (ping-pong) */
    EXT_MORPH_DEST,    /* the kit the LXR-02 morphs to (kept with the kit; the plugin morphs to the loaded morph kit) */
    LXR_NUM_EXT
};
#define LXR_KIT_BYTES (LXR_NUM_SOUND_PARAMS + LXR_NUM_EXT)   /* 247: END_OF_SOUND_PARAMETERS */

/* the audio output values of a voice's "out" (and the FX's) */
enum { OUT_ST1 = 0, OUT_ST2, OUT_L1, OUT_R1, OUT_L2, OUT_R2, OUT_FX, OUT_NUM };

#define LXR_BLOCK 64   /* the firmware's block (OUTPUT_DMA_SIZE) */

void lxr_dsp_init(void);                                   /* power-up: every voice, the FX bus, the parameter table */
void lxr_dsp_setTempo(float bpm);                          /* for synced LFOs */
/* one kit parameter (0..246) as the menu sends it: mod targets to the modulation nodes, the rest through the handler */
void lxr_dsp_setParam(uint16_t par, uint8_t value);
void lxr_dsp_setExt(uint8_t ext, uint8_t value);           /* = lxr_dsp_setParam(228 + ext, value) */
/* the whole kit, blended with the morph kit by `morph` (0..255) */
void lxr_dsp_applyKit(const uint8_t *kit, const uint8_t *morphKit, uint8_t morph);
uint8_t lxr_dsp_morphValue(const uint8_t *kit, const uint8_t *morphKit, uint16_t par, uint8_t morph);
/* trigger track 0..6 (6 = open hi-hat) with velocity 0..127 and note (63 = no transpose) */
void lxr_dsp_trigger(uint8_t track, uint8_t vel, uint8_t note);
/* step automation (parameter locks), per track: two lanes, the firmware's automation nodes */
void lxr_dsp_automation(uint8_t track, uint8_t lane, uint8_t param1, uint8_t value);   /* param1 = parameter + 1, 0 = none */
void lxr_dsp_automationReset(void);
void lxr_dsp_globalDecimation(uint8_t value);              /* performance page "sr" */
/* LXR_BLOCK frames of the stereo out (OUT 1 + OUT 2), float -1..1; fader gain 0..1 per voice (6, tapered) */
void lxr_dsp_render(float *outL, float *outR, const float fader[6]);
void lxr_dsp_silenceFx(void);                              /* Panic / reset: the delay and reverb tails */
uint8_t lxr_dsp_voiceOfTrack(uint8_t track);

/* the 0.37 name of parameter 127 (the firmware calls it PAR_BANK_CHANGE): no sound, never sent */
#define PAR_RESERVED4 PAR_BANK_CHANGE

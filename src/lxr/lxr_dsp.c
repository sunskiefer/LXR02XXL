/* LXRXXL -- the LXR-02 sound engine glue: parameters, triggers and the render call.
 * Compiled with -include lxr_state.h like the vendored engine: its globals belong to the per-instance state.
 *
 * Everything that makes sound is the LXR firmware's own code (Copyright (C) 2012-2026 Erica Synths, GPL-3.0-only,
 * third_party/lxr-desktop-app): parameters go through its handler (cc_handler.inc, extracted verbatim), kits are sent
 * in its order (Parameters.c parameterOrder, FX first), the block is its mixer (mixer.c) with its FX bus (FxBus.c). */
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "lxr_dsp.h"

#include "Soundengine/config.h"
#include "Soundengine/globals.h"
#include "Parameters.h"
#include "Soundengine/MIDI/MidiMessages.h"
#include "Soundengine/MIDI/MidiParser.h"
#include "Soundengine/MIDI/ParameterArray.h"
#include "Soundengine/MIDI/valueShaper.h"
#include "Soundengine/DSPAudio/DrumVoice.h"
#include "Soundengine/DSPAudio/Snare.h"
#include "Soundengine/DSPAudio/CymbalVoice.h"
#include "Soundengine/DSPAudio/HiHat.h"
#include "Soundengine/DSPAudio/mixer.h"
#include "Soundengine/DSPAudio/modulationNode.h"
#include "Soundengine/DSPAudio/automationNode.h"
#include "Soundengine/DSPAudio/BufferTools.h"
#include "Soundengine/DSPAudio/lfo.h"
#include "Soundengine/DSPAudio/random.h"
#include "Soundengine/DSPAudio/FX/FxRamBuffer.h"
#include "Soundengine/FxBus.h"
#include "menu/menu.h"
#include "menu/CcNr2Text.h"
#include "main.h"

_Static_assert(LXR_KIT_BYTES == END_OF_SOUND_PARAMETERS, "a kit is the firmware's sound parameters");
_Static_assert(LXR_NUM_SOUND_PARAMS + EXT_PWM1 == PAR_PWM_1 && LXR_NUM_SOUND_PARAMS + EXT_FX_TYPE == PAR_FX_TYPE &&
               LXR_NUM_SOUND_PARAMS + EXT_FX_REVERBTYPE == PAR_FX_REVERB_TYPE &&
               LXR_NUM_SOUND_PARAMS + EXT_MORPH_DEST == PAR_MORPH_KIT_DEST, "the kit's extension is in firmware order");
_Static_assert(LXR_BLOCK == OUTPUT_DMA_SIZE, "one render is one firmware block");

/* ---- what the handler and the engine take from the rest of the firmware (menu, sequencer, front panel, pots) ---- */
uint8_t activeParameters[NUM_PARAMS];     /* the menu's edit buffer: the kit as last sent */
volatile uint32_t systick_ticks;
volatile uint8_t bCurrentSampleValid;
LxrGpio lxr_jacks = {1u};                 /* every output jack plugged (main.h) */
uint8_t menu_activePage = 0;
/* MidiParser.c's own state that the handler uses (above the extracted part) */
#define SEMITONE_UP 1.0594630943592952645618252949463f
#define SEMITONE_DOWN 0.94387431268169349664191315666753f
static uint16_t midiParser_activeNrpnNumber = 0;
uint8_t midiParser_originalCcValues[0xff];
uint8_t midiParser_selectedLfoVoice[6];
static float g_fader[6] = {1, 1, 1, 1, 1, 1};
static float g_bpm = 120.f;
static float g_dcXL, g_dcYL, g_dcXR, g_dcYR;   /* the output DC blocker */
static uint8_t g_mutes;                   /* the firmware's mute CCs (CC2_MUTE_n) land here; the sequencer keeps its own */

float adc_getFaderVolume(uint8_t idx) { return idx < 6 ? g_fader[idx] : 1.f; }
float lxr_dsp_getBpm(void) { return g_bpm; }
uint16_t seq_getBpm(void) { return (uint16_t)(g_bpm + 0.5f); }
void seq_setMute(uint8_t track, uint8_t muted) { if (muted) g_mutes |= 1u << track; else g_mutes &= ~(1u << track); }
uint8_t menu_getSubPage(void) { return 0; }
void menu_switchSubPage(uint8_t subPageNr, uint8_t force) { (void)subPageNr; (void)force; }
void menu_notifyExternalParamChanged(uint16_t p) { (void)p; }
void preset_morph(uint8_t morph) { (void)morph; }    /* CC1 -> morph is the plugin's (engine.c) */

/* the firmware's parameter handler, verbatim */
#include "cc_handler.inc"

/* ---- automation nodes: two lanes per track, as sequencer.c keeps them ---- */
static AutomationNode lxr_autoNodes[7][2];

void lxr_dsp_init(void)
{
    uint8_t i;
    /* soundengine_init() (Soundengine.c), without the codec and the sequencer */
    initRng();
    mixer_init();          /* the decimators, the routing, FxBus_init() */
    parameterArray_init();
    for (i = 0; i < 6; i++) modNode_init(&velocityModulators[i]);
    memset(midi_MidiChannels, 0, 8);
    initDrumVoice();
    Snare_init();
    HiHat_init();
    Cymbal_init();
    paramToModTargetInit();   /* menu.c */
    memset(FX_BUFFER, 0, sizeof FX_BUFFER);
    for (i = 0; i < 7; i++) {
        autoNode_init(&lxr_autoNodes[i][0]);
        autoNode_init(&lxr_autoNodes[i][1]);
    }
}

void lxr_dsp_setTempo(float bpm)
{
    if (bpm < 1.f) bpm = 1.f;
    if (bpm != g_bpm) {
        g_bpm = bpm;
        lfo_recalcSync();
    }
}

/* menu -> engine: CC n sets parameter n - 1 (frontPanel_sendData(MIDI_CC) adds one), CC2 n parameter 128 + n */
static void send_cc(uint16_t par, uint8_t value)
{
    MidiMsg msg;
    if (par < 128u) {
        msg.status = MIDI_CC;
        msg.data1 = (uint8_t)((par + 1u) & 0x7fu);
    } else {
        msg.status = MIDI_CC2;
        msg.data1 = (uint8_t)(par - 128u);
    }
    msg.data2 = value;
    midiParser_ccHandler(msg, 1, 0, 0.f);
}

static void set_lfo_target(uint8_t lfo, uint16_t target)
{
    /* FRONT_CC_LFO_TARGET (CortexParser.c) */
    switch (lfo) {
    case 0: case 1: case 2: modNode_setDestination(&voiceArray[lfo].lfo.modTarget, target); break;
    case 3: modNode_setDestination(&snareVoice.lfo.modTarget, target); break;
    case 4: modNode_setDestination(&cymbalVoice.lfo.modTarget, target); break;
    case 5: modNode_setDestination(&hatVoice.lfo.modTarget, target); break;
    default: break;
    }
}

static uint8_t num_targets(void) { return getNumModTargets(); }

/* The menu never sends a value past a parameter's range (menu_encoderChangeParameter); MIDI, automation and old data
 * could. Past the six waveforms the firmware would play user samples, which it leaves unrendered (stack garbage as an
 * FM index): values are kept in range here, as the menu keeps them. */
uint8_t meta_soundMax(uint16_t par);
static uint8_t in_range(uint16_t par, uint8_t value)
{
    const uint8_t mx = meta_soundMax(par);
    return value > mx ? mx : value;
}

void lxr_dsp_setParam(uint16_t par, uint8_t value)
{
    if (par >= END_OF_SOUND_PARAMETERS) return;
    value = in_range(par, value);
    activeParameters[par] = value;
    if (par == PAR_BANK_CHANGE) return;   /* CC 128 does not exist (presetManager.c skips it too) */
    if (par == PAR_NONE) return;          /* CC 1, the mod wheel: the plugin's morph (engine.c), not a sound */
    /* preset_sendDrumsetParameters(): the kit keeps mod targets as indices into modTargets, the engine takes the
     * parameter (FRONT_CC_VELO_TARGET / FRONT_CC_LFO_TARGET) */
    if (par >= PAR_VEL_DEST_1 && par <= PAR_VEL_DEST_6) {
        if (value >= num_targets()) value = 0;
        modNode_setDestination(&velocityModulators[par - PAR_VEL_DEST_1], modTargets[value].param);
        return;
    }
    if (par >= PAR_TARGET_LFO1 && par <= PAR_TARGET_LFO6) {
        if (value >= num_targets()) value = 0;
        set_lfo_target((uint8_t)(par - PAR_TARGET_LFO1), modTargets[value].param);
        return;
    }
    if (par >= PAR_VOICE_LFO1 && par <= PAR_VOICE_LFO6)
        return;   /* only selects which voice's list the target comes from (ui.c) */
    if (par == PAR_MORPH_KIT_DEST)
        return;   /* the hardware's morph kit slot: the plugin morphs to the morph kit it loaded */
    send_cc(par, value);
}

void lxr_dsp_setExt(uint8_t ext, uint8_t value)
{
    if (ext < LXR_NUM_EXT) lxr_dsp_setParam((uint16_t)(LXR_NUM_SOUND_PARAMS + ext), value);
}

/* presetManager.c: interpolate(); the plugin switches option values halfway instead of passing through other options,
 * and keeps the source kit's audio routing (preset_morph_tick) */
static uint8_t interpolate(uint8_t a, uint8_t b, uint8_t x)
{
    uint16_t fixedPointValue = (uint16_t)(((a * 256) + (b - a) * x));
    uint8_t result = (uint8_t)(fixedPointValue / 256);
    return (uint8_t)((fixedPointValue & 0xff) < 0x7f ? result : result + 1);
}

static uint8_t morph_skips(uint16_t index)
{
    if (index == PAR_BANK_CHANGE || index == PAR_MORPH_KIT_DEST) return 1;
    if (index >= PAR_AUDIO_OUT1 && index <= PAR_AUDIO_OUT6) return 1;
    if (index >= PAR_VEL_DEST_1 && index <= PAR_VEL_DEST_6) return 1;
    if (index >= PAR_VOICE_LFO1 && index <= PAR_VOICE_LFO6) return 1;
    if (index >= PAR_TARGET_LFO1 && index <= PAR_TARGET_LFO6) return 1;
    if (index == PAR_FX_ROUTING) return 1;
    return 0;
}

uint8_t lxr_dsp_morphValue(const uint8_t *kit, const uint8_t *morphKit, uint16_t par, uint8_t morph)
{
    if (!morphKit || morph == 0 || par >= END_OF_SOUND_PARAMETERS || morph_skips(par)) return kit[par];
    {
        const uint8_t dt = (uint8_t)(parameter_dtypes[par] & 0xFF);
        if (dt == DTYPE_MENU || dt == DTYPE_ON_OFF || dt == DTYPE_NOTE_NAME || dt == DTYPE_VOICE_LFO)
            return morph < 128 ? kit[par] : morphKit[par];
    }
    return interpolate(kit[par], morphKit[par], morph);
}

void lxr_dsp_applyKit(const uint8_t *kit, const uint8_t *morphKit, uint8_t morph)
{
    uint16_t i;
    /* preset_sendDrumsetParameters(): the mod targets first, then every parameter in the firmware's order (the FX
     * unit first, against clicks) */
    for (i = 0; i < 6; i++) {
        lxr_dsp_setParam((uint16_t)(PAR_VEL_DEST_1 + i), kit[PAR_VEL_DEST_1 + i]);
        lxr_dsp_setParam((uint16_t)(PAR_TARGET_LFO1 + i), kit[PAR_TARGET_LFO1 + i]);
    }
    for (i = 0; i < END_OF_SOUND_PARAMETERS; i++) {
        const uint16_t p = parameterOrder[i];
        if (p >= END_OF_SOUND_PARAMETERS) continue;
        if ((p >= PAR_VEL_DEST_1 && p <= PAR_VEL_DEST_6) || (p >= PAR_TARGET_LFO1 && p <= PAR_TARGET_LFO6)) continue;
        lxr_dsp_setParam(p, lxr_dsp_morphValue(kit, morphKit, p, morph));
    }
}

uint8_t lxr_dsp_voiceOfTrack(uint8_t track) { return track > 5 ? 5 : track; }

/* voiceControl_triggerNow(), MidiVoiceControl.c */
void lxr_dsp_trigger(uint8_t track, uint8_t vel, uint8_t note)
{
    if (track < 3) Drum_trigger(track, vel, note);
    else if (track < 4) Snare_trigger(vel, note);
    else if (track < 5) Cymbal_trigger(vel, note);
    else if (track < 7) HiHat_trigger(vel, (uint8_t)(track - 5), note);
}

void lxr_dsp_automation(uint8_t track, uint8_t lane, uint8_t param1, uint8_t value)
{
    uint16_t dest = NO_AUTOMATION;
    if (track > 6 || lane > 1) return;
    if (param1) {
        const uint16_t par = (uint16_t)(param1 - 1u);
        /* the automation node's destination is the firmware's step encoding: CC number (param + 1) below 128,
         * the parameter number from 128 (automationNode.c) */
        if (par >= END_OF_SOUND_PARAMETERS) return;
        value = in_range(par, value);
        dest = par < 128u ? (uint16_t)(par + 1u) : par;
    }
    autoNode_setDestination(&lxr_autoNodes[track][lane], dest);
    autoNode_updateValue(&lxr_autoNodes[track][lane], value);
}

void lxr_dsp_automationReset(void)
{
    uint8_t i;
    for (i = 0; i < 7; i++) {
        autoNode_setDestination(&lxr_autoNodes[i][0], NO_AUTOMATION);
        autoNode_setDestination(&lxr_autoNodes[i][1], NO_AUTOMATION);
    }
}

void lxr_dsp_globalDecimation(uint8_t value) { send_cc(PAR_VOICE_DECIMATION_ALL, value); }

void lxr_dsp_silenceFx(void)
{
    /* the FX bus's memory (delay line, reverb tanks) and its state, as at power-up; the kit's FX settings again */
    uint16_t p;
    memset(FX_BUFFER, 0, sizeof FX_BUFFER);
    FxBus_init();
    for (p = PAR_FX_TYPE; p <= PAR_FX_DELAY_TYPE; p++) send_cc(p, activeParameters[p]);
}

/* The plugin has one stereo output, MPC's track out: the LXR-02's OUT 1 and OUT 2 added (each mono routing stays on
 * its side). The voices read their fader through adc_getFaderVolume() and smooth it themselves, as on the machine. */
void lxr_dsp_render(float *outL, float *outR, const float fader[6])
{
    int16_t out1[LXR_BLOCK * 2], out2[LXR_BLOCK * 2];
    int i;
    for (i = 0; i < 6; i++) g_fader[i] = fader[i] < 0.f ? 0.f : fader[i] > 1.f ? 1.f : fader[i];
    mixer_calcNextSampleBlock(out1, out2);
    /* the LXR-02's outputs are AC coupled: a DC blocker (one pole at ~7 Hz) takes away what the coupling capacitors
     * take away there, e.g. the tube drive's bias (ecc83Table is not zero at zero input) */
    for (i = 0; i < LXR_BLOCK; i++) {
        const float l = ((float)out1[2 * i] + (float)out2[2 * i]) * (1.f / 32768.f);
        const float r = ((float)out1[2 * i + 1] + (float)out2[2 * i + 1]) * (1.f / 32768.f);
        g_dcYL = l - g_dcXL + 0.999f * g_dcYL; g_dcXL = l;
        g_dcYR = r - g_dcXR + 0.999f * g_dcYR; g_dcXR = r;
        outL[i] = g_dcYL;
        outR[i] = g_dcYR;
    }
}

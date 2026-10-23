/* LXRXXL -- parameter names and value text, as the LXR shows them (the LXR firmware's menu tables: MenuText.h and
 * menu.c, Copyright (C) 2012-2026 Erica Synths, GPL-3.0-only). */
#include <stdio.h>
#include <string.h>
#include "lxr.h"
#include "Parameters.h"
#include "menu/menu.h"
#include "menu/MenuText.h"
#include "menu/CcNr2Text.h"

#include "lxr/menu_tables.inc"

const char *meta_textShort(uint8_t t) { return t < NUM_NAMES ? shortNames[valueNames[t].shortName] : ""; }
const char *meta_textLong(uint8_t t) { return t < NUM_NAMES ? longNames[valueNames[t].longName] : ""; }
const char *meta_textCat(uint8_t t) { return t < NUM_NAMES ? catNames[valueNames[t].category] : ""; }

const char *meta_targetLong(uint8_t idx)
{
    if (idx >= getNumModTargets()) return "";
    return longNames[valueNames[modTargets[idx].nameIdx].longName];
}

uint8_t meta_numModTargets(void) { return getNumModTargets(); }
uint16_t meta_modTargetParam(uint8_t idx) { return idx < getNumModTargets() ? modTargets[idx].param : PAR_NONE; }

void meta_modTargetRange(uint8_t voice0, uint8_t *start, uint8_t *end)
{
    if (voice0 > 5) voice0 = 5;
    *start = modTargetVoiceOffsets[voice0].start;
    *end = modTargetVoiceOffsets[voice0].end;
}

uint8_t meta_paramToModTarget(uint16_t par)
{
    uint8_t i, n = getNumModTargets();
    for (i = 1; i < n; i++)
        if (modTargets[i].param == par) return i;
    return 0;
}

/* menu_displayModTargetShort(): "off", or the target's short name, with its voice number in front when asked */
void meta_modTargetText(uint8_t idx, char *buf, int withVoice)
{
    if (idx >= getNumModTargets() || modTargets[idx].param == PAR_NONE) { strcpy(buf, "off"); return; }
    if (withVoice) {
        uint8_t v = 0, i;
        for (i = 0; i < 6; i++)
            if (idx >= modTargetVoiceOffsets[i].start && idx <= modTargetVoiceOffsets[i].end) v = (uint8_t)(i + 1);
        if (v) { sprintf(buf, "%d%s", v, shortNames[valueNames[modTargets[idx].nameIdx].shortName]); return; }
    }
    strcpy(buf, shortNames[valueNames[modTargets[idx].nameIdx].shortName]);
}

/* setNoteName(), menu.c */
void meta_noteName(uint8_t num, char *buf)
{
    uint8_t n = num % 12;
    buf[0] = (char)((n > 8 ? 60 : 67) + ((n + (n > 4)) / 2));
    buf[1] = ((n < 5 && (n & 1)) || (n > 5 && !(n & 1))) ? '#' : 0;
    if (buf[1]) { sprintf(buf + 2, "%d", num / 12); } else { sprintf(buf + 1, "%d", num / 12); }
}

/* getMaxEntriesForMenu() / getMenuItemNameForValue() (menu.c); no user samples in the plugin */
#define MENU_LIST(id, tbl) case id: return (v < 0) ? (const char *)(uintptr_t)(tbl)[0][0] : (tbl)[v + 1];
static const char *menu_lookup(uint8_t menuId, int v)
{
    switch (menuId) {
    MENU_LIST(MENU_TRANS, transientNames)
    MENU_LIST(MENU_AUDIO_OUT, outputNames)
    MENU_LIST(MENU_AUDIO_OUT_FX, outputNamesFx)
    MENU_LIST(MENU_FILTER, filterTypes)
    MENU_LIST(MENU_WAVEFORM, waveformNames)
    MENU_LIST(MENU_SYNC_RATES, syncRateNames)
    MENU_LIST(MENU_LFO_WAVES, lfoWaveNames)
    MENU_LIST(MENU_RETRIGGER, retriggerNames)
    MENU_LIST(MENU_FOLLOW, followNames)
    MENU_LIST(MENU_SEQ_QUANT, quantisationNames)
    MENU_LIST(MENU_MIDI, midiModes)
    MENU_LIST(MENU_NEXT_PATTERN, nextPatternNames)
    MENU_LIST(MENU_ROLL_RATES, rollRateNames)
    MENU_LIST(MENU_MIDI_ROUTING, midiRoutingNames)
    MENU_LIST(MENU_MIDI_FILTERING, midiFilterNames)
    MENU_LIST(MENU_PPQ_IN, ppqInNames)
    MENU_LIST(MENU_PPQ_OUT, ppqOutNames)
    MENU_LIST(MENU_MIX_FM, mixFmNames)
    MENU_LIST(MENU_FX, fxNames)
    MENU_LIST(MENU_DIST, distNames)
    MENU_LIST(MENU_RATIO, ratioNames)
    MENU_LIST(MENU_DELAY, delayNames)
    MENU_LIST(MENU_REVERB, reverbNames)
    MENU_LIST(MENU_PAT_MODE, patModeNames)
    MENU_LIST(MENU_STEP_FLAM, stepFlamNames)
    MENU_LIST(MENU_STEP_SHIFT, stepShiftNames)
    default: return v < 0 ? (const char *)(uintptr_t)1 : "";
    }
}
static uint8_t menu_entries(uint8_t menuId) { return (uint8_t)(uintptr_t)menu_lookup(menuId, -1); }
static const char *menu_name(uint8_t menuId, uint8_t v)
{
    const uint8_t n = menu_entries(menuId);
    return menu_lookup(menuId, v < n ? v : 0);
}

/* the largest value of a sound parameter: menu_encoderChangeParameter()'s clamps. The mod target selections are
 * ranges of the target list; their slots (ui.c) map them separately. */
uint8_t meta_soundMax(uint16_t par)
{
    if (par >= END_OF_SOUND_PARAMETERS) return 127;
    switch (parameter_dtypes[par] & 0xFF) {
    case DTYPE_0B255: return 255;
    case DTYPE_1B16: return 16;
    case DTYPE_1B64: return 64;
    case DTYPE_0B15: return 15;
    case DTYPE_0B100: return 100;
    case DTYPE_ON_OFF: case DTYPE_0b1: return 1;
    case DTYPE_VOICE_LFO: return 6;
    case DTYPE_MENU: return (uint8_t)(menu_entries((uint8_t)(parameter_dtypes[par] >> 8)) - 1);
    case DTYPE_TARGET_SELECTION_LFO: case DTYPE_TARGET_SELECTION_VELO: case DTYPE_AUTOM_TARGET:
        return (uint8_t)(getNumModTargets() - 1);
    default: return 127;
    }
}

/* the value as the LCD shows it (menu_repaintGeneric) */
void meta_soundText(uint16_t par, uint8_t v, char *buf)
{
    if (par >= END_OF_SOUND_PARAMETERS) { sprintf(buf, "%d", v); return; }
    switch (parameter_dtypes[par] & 0xFF) {
    case DTYPE_TARGET_SELECTION_VELO:
    case DTYPE_TARGET_SELECTION_LFO:
        meta_modTargetText(v, buf, 0);
        break;
    case DTYPE_ON_OFF: strcpy(buf, v == 1 ? "on" : "off"); break;
    case DTYPE_MENU: {
        const char *s = menu_name((uint8_t)(parameter_dtypes[par] >> 8), v);
        snprintf(buf, 8, "%s", s);
        while (*buf && buf[strlen(buf) - 1] == ' ') buf[strlen(buf) - 1] = 0;
        break;
    }
    case DTYPE_PM63: sprintf(buf, "%d", (int)v - 63); break;
    case DTYPE_NOTE_NAME:
        meta_noteName(v, buf);
        break;
    case DTYPE_0b1: sprintf(buf, "%d", v + 1); break;
    case DTYPE_VOICE_LFO: sprintf(buf, "v%d", v); break;
    default: sprintf(buf, "%d", v); break;
    }
}

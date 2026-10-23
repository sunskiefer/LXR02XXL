/* LXRXXL -- the LXR-02 front panel (owner's manual ch. 2 and 7): modes, buttons, SHIFT functions, LEDs, the display and
 * the screen's sliders (the values the LCD shows, one slider each).
 *
 * Touch has no "hold": a tap is a press and a release at once. So SHIFT and COPY latch on the screen (tap on, tap off;
 * their LEDs show it), LOAD and SAVE wait for their data-type key, and manual rolls toggle. The Force's pads send real
 * presses and releases (ui_padNote), so on the pads everything works held, exactly as on the machine. */
#include <stdio.h>
#include <string.h>
#include "lxr.h"
#include "menu.h"
#include "ParameterArray.h"

/* ---------------------------------------------------------------- slots */
enum {
    SK_NONE = 0,
    SK_SOUND,      /* id: kit parameter */
    SK_EXT,        /* id: kit extension (pwm) */
    SK_FX,         /* id: kit extension (FX) */
    SK_LFOVOICE,   /* id: lfo 0..5 */
    SK_LFODST,     /* id: lfo 0..5 */
    SK_VELDST,     /* id: voice 0..5 */
    SK_TRKLEN,     /* the active track's length */
    SK_TRKCH,      /* id: track: MIDI channel */
    SK_TRKNOTE,    /* id: track: MIDI note */
    SK_STEP,       /* id: 0 vel 1 note 2 prob 3 p1d 4 p1v 5 p2d 6 p2v */
    SK_GLOB,       /* id: see glob_* below */
    SK_MORPH,
    SK_GEN,        /* id: 0 len 1 steps 2 rot */
    SK_LSSLOT,     /* load/save: the slot */
    SK_NAMEPOS, SK_NAMECASE, SK_NAMECHR,
    SK_CLEAR,
    SK_SONGROW, SK_SONGPAT,
    SK_LAST,       /* last step */
};
enum { G_BPM, G_CH, G_MTX, G_MRX, G_FLW, G_KCM, G_PAD, G_LRN, G_ROLL, G_SHU, G_TRK, G_QNT };

static const char *const trackNames[NUM_TRACKS] = {"DRUM1", "DRUM2", "DRUM3", "SNARE", "CLP/CYM", "CL HH", "OP HH"};
const char *ui_trackName(int t) { return t >= 0 && t < NUM_TRACKS ? trackNames[t] : "?"; }
static const char *const sectionNames[8] = {"OSC", "AEG", "MOD", "FM", "CLICK", "FILTER", "LFO", "MIX"};
static const char *const lsNames[5] = {"Kit", "MorphKit", "Song", "Project", "Pattern"};
static const char *const clearNames[5] = {"kit", "track", "pattern", "autom. 1", "autom. 2"};
static const char *const fxTypes[6] = {"off", "drv", "rng", "cmp", "del", "rev"};
static const char *const outNames[6] = {"St1", "St2", "L1", "R1", "L2", "R2"};
static const char *const drvTypes[3] = {"tub", "fld", "clp"};
static const char *const ringWaves[6] = {"Sin", "Tri", "Saw", "Rec", "Noi", "PWM"};
static const char *const ratioNames[6] = {"1:1", "2:1", "3:1", "4:1", "6:1", "8:1"};
static const char *const delayTypes[2] = {"Del", "Pp"};
static const char *const rollNames[14] = {"One", "1/1", "1/2", "1/3", "1/4", "1/6", "1/8", "12", "16", "24", "32", "48", "64", "128"};
static const char *const filtNames[16] = {"off", "N", "R", "RN", "C", "CN", "CR", "CRN", "P", "PN", "PR", "PRN", "PC", "PCN", "PCR", "all"};
static const char *const quantNames[5] = {"off", "8", "16", "32", "64"};

#define SLOT(k, i) ((Slot){(uint8_t)(k), (uint8_t)(i)})


/* a free-running frame clock for blinking and messages (engine.c advances it) */
extern uint32_t lxr_clock(Lxr *x);

static void set_pages_voice(Lxr *x)
{
    const uint8_t t = x->ui.track, v = t > 5 ? 5 : t;
    Slot *s = x->slots;
    int i;
    for (i = 0; i < NUM_SLOTS; i++) s[i] = SLOT(SK_NONE, 0);
    switch (x->ui.section) {
    case 0:   /* OSC */
        s[0] = SLOT(SK_SOUND, PAR_COARSE1 + 2 * v);
        s[1] = SLOT(SK_SOUND, PAR_FINE1 + 2 * v);
        if (v == 3) {
            s[2] = SLOT(SK_SOUND, PAR_NOISE_FREQ1);
            s[3] = SLOT(SK_SOUND, PAR_MIX1);
            s[4] = SLOT(SK_SOUND, PAR_OSC_WAVE_SNARE);
            s[5] = SLOT(SK_EXT, EXT_PWM4);
        } else {
            s[2] = SLOT(SK_SOUND, v < 3 ? PAR_OSC_WAVE_DRUM1 + v : v == 4 ? PAR_WAVE1_CYM : PAR_WAVE1_HH);
            s[3] = SLOT(SK_EXT, EXT_PWM1 + v);
        }
        break;
    case 1:   /* AEG */
        if (v == 5) {
            s[0] = SLOT(SK_SOUND, PAR_VELOA6);
            s[1] = SLOT(SK_SOUND, PAR_VELOD6_CLOSED);
            s[2] = SLOT(SK_SOUND, PAR_VELOD6_OPEN);
            s[3] = SLOT(SK_SOUND, PAR_VOL_SLOPE6);
        } else {
            s[0] = SLOT(SK_SOUND, PAR_VELOA1 + 2 * v);
            s[1] = SLOT(SK_SOUND, PAR_VELOD1 + 2 * v);
            if (v == 3 || v == 4) {
                s[2] = SLOT(SK_SOUND, v == 3 ? PAR_REPEAT4 : PAR_REPEAT5);
                s[3] = SLOT(SK_SOUND, PAR_VOL_SLOPE1 + v);
            } else
                s[2] = SLOT(SK_SOUND, PAR_VOL_SLOPE1 + v);
        }
        break;
    case 2:   /* MOD */
        if (v <= 3) {
            s[0] = SLOT(SK_SOUND, PAR_MOD_EG1 + v);
            s[1] = SLOT(SK_SOUND, PAR_PITCH_SLOPE1 + v);
            s[2] = SLOT(SK_SOUND, PAR_MODAMNT1 + v);
            s[4] = SLOT(SK_VELDST, v);
            s[5] = SLOT(SK_SOUND, PAR_VELO_MOD_AMT_1 + v);
            s[6] = SLOT(SK_SOUND, PAR_VOLUME_MOD_ON_OFF1 + v);
        } else {
            s[0] = SLOT(SK_VELDST, v);
            s[1] = SLOT(SK_SOUND, PAR_VELO_MOD_AMT_1 + v);
            s[2] = SLOT(SK_SOUND, PAR_VOLUME_MOD_ON_OFF1 + v);
        }
        break;
    case 3:   /* FM */
        if (v < 3) {
            s[0] = SLOT(SK_SOUND, PAR_FMAMNT1 + 2 * v);
            s[1] = SLOT(SK_SOUND, PAR_FM_FREQ1 + 2 * v);
            s[2] = SLOT(SK_SOUND, PAR_MOD_WAVE_DRUM1 + v);
            s[3] = SLOT(SK_SOUND, PAR_MIX_MOD_1 + v);
        } else if (v == 4) {
            s[0] = SLOT(SK_SOUND, PAR_MOD_OSC_F1_CYM);
            s[1] = SLOT(SK_SOUND, PAR_MOD_OSC_F2_CYM);
            s[2] = SLOT(SK_SOUND, PAR_MOD_OSC_GAIN1_CYM);
            s[3] = SLOT(SK_SOUND, PAR_MOD_OSC_GAIN2_CYM);
            s[4] = SLOT(SK_SOUND, PAR_WAVE2_CYM);
            s[5] = SLOT(SK_SOUND, PAR_WAVE3_CYM);
        } else if (v == 5) {
            s[0] = SLOT(SK_SOUND, PAR_MOD_OSC_F1);
            s[1] = SLOT(SK_SOUND, PAR_MOD_OSC_F2);
            s[2] = SLOT(SK_SOUND, PAR_MOD_OSC_GAIN1);
            s[3] = SLOT(SK_SOUND, PAR_MOD_OSC_GAIN2);
            s[4] = SLOT(SK_SOUND, PAR_WAVE2_HH);
            s[5] = SLOT(SK_SOUND, PAR_WAVE3_HH);
        }   /* the snare has no FM: the page is empty (manual) */
        break;
    case 4:   /* CLICK */
        s[0] = SLOT(SK_SOUND, PAR_TRANS1_WAVE + v);
        s[1] = SLOT(SK_SOUND, PAR_TRANS1_VOL + v);
        s[2] = SLOT(SK_SOUND, PAR_TRANS1_FREQ + v);
        break;
    case 5:   /* FILTER */
        s[0] = SLOT(SK_SOUND, PAR_FILTER_FREQ_1 + v);
        s[1] = SLOT(SK_SOUND, PAR_RESO_1 + v);
        s[2] = SLOT(SK_SOUND, PAR_FILTER_TYPE_1 + v);
        s[3] = SLOT(SK_SOUND, PAR_FILTER_DRIVE_1 + v);
        break;
    case 6:   /* LFO */
        s[0] = SLOT(SK_SOUND, PAR_FREQ_LFO1 + v);
        s[1] = SLOT(SK_SOUND, PAR_SYNC_LFO1 + v);
        s[2] = SLOT(SK_SOUND, PAR_AMOUNT_LFO1 + v);
        s[3] = SLOT(SK_SOUND, PAR_WAVE_LFO1 + v);
        s[4] = SLOT(SK_SOUND, PAR_RETRIGGER_LFO1 + v);
        s[5] = SLOT(SK_SOUND, PAR_OFFSET_LFO1 + v);
        s[6] = SLOT(SK_LFOVOICE, v);
        s[7] = SLOT(SK_LFODST, v);
        break;
    default:  /* MIX */
    {
        static const uint8_t pans[6] = {PAR_PAN1, PAR_PAN2, PAR_PAN3, PAR_PAN4, PAR_PAN5, PAR_PAN6};
        s[0] = SLOT(SK_SOUND, PAR_VOL1 + v);
        s[1] = SLOT(SK_SOUND, pans[v]);
        s[2] = SLOT(SK_SOUND, PAR_VOICE_DECIMATION1 + v);
        s[3] = SLOT(SK_SOUND, PAR_DRIVE1 + v);
        s[4] = SLOT(SK_SOUND, PAR_AUDIO_OUT1 + v);
        s[5] = SLOT(SK_TRKLEN, 0);
        s[6] = SLOT(SK_TRKCH, t);
        s[7] = SLOT(SK_TRKNOTE, t);
        break;
    }
    }
}

void ui_refreshSlots(Lxr *x)
{
    Slot *s = x->slots;
    int i;
    for (i = 0; i < NUM_SLOTS; i++) s[i] = SLOT(SK_NONE, 0);
    switch (x->ui.mode) {
    case M_VOICE: set_pages_voice(x); break;
    case M_STEP:
        s[0] = SLOT(SK_STEP, 0); s[1] = SLOT(SK_STEP, 1); s[2] = SLOT(SK_STEP, 2);
        s[4] = SLOT(SK_STEP, 3); s[5] = SLOT(SK_STEP, 4); s[6] = SLOT(SK_STEP, 5); s[7] = SLOT(SK_STEP, 6);
        break;
    case M_PERF:
        s[0] = SLOT(SK_GLOB, G_BPM); s[1] = SLOT(SK_GLOB, G_ROLL); s[2] = SLOT(SK_MORPH, 0);
        s[3] = SLOT(SK_SOUND, PAR_VOICE_DECIMATION_ALL); s[4] = SLOT(SK_GLOB, G_SHU);
        break;
    case M_PATGEN:
        s[0] = SLOT(SK_GEN, 0); s[1] = SLOT(SK_GEN, 1); s[2] = SLOT(SK_GEN, 2); s[3] = SLOT(SK_MORPH, 0);
        break;
    case M_CONFIG:
        s[0] = SLOT(SK_GLOB, G_BPM); s[1] = SLOT(SK_GLOB, G_CH); s[2] = SLOT(SK_GLOB, G_MTX); s[3] = SLOT(SK_GLOB, G_MRX);
        s[4] = SLOT(SK_GLOB, G_FLW); s[5] = SLOT(SK_GLOB, G_KCM); s[6] = SLOT(SK_GLOB, G_PAD); s[7] = SLOT(SK_GLOB, G_LRN);
        break;
    case M_FX: {
        const uint8_t type = x->kit.b[LXR_NUM_SOUND_PARAMS + EXT_FX_TYPE];
        s[0] = SLOT(SK_FX, EXT_FX_TYPE); s[1] = SLOT(SK_FX, EXT_FX_OUT); s[2] = SLOT(SK_FX, EXT_FX_DW);
        if (type == 1) {
            s[3] = SLOT(SK_FX, EXT_FX_DRVTYPE);
            s[4] = SLOT(SK_FX, EXT_FX_P1); s[5] = SLOT(SK_FX, EXT_FX_P2); s[6] = SLOT(SK_FX, EXT_FX_P3); s[7] = SLOT(SK_FX, EXT_FX_P4);
        } else if (type == 2) {
            s[3] = SLOT(SK_FX, EXT_FX_RINGWAVE); s[4] = SLOT(SK_FX, EXT_FX_P1);
        } else if (type == 3) {
            s[3] = SLOT(SK_FX, EXT_FX_RATIO);
            s[4] = SLOT(SK_FX, EXT_FX_P1); s[5] = SLOT(SK_FX, EXT_FX_P2); s[6] = SLOT(SK_FX, EXT_FX_P3); s[7] = SLOT(SK_FX, EXT_FX_P4);
        } else if (type == 4) {
            s[3] = SLOT(SK_FX, EXT_FX_DELAYTYPE);
            s[4] = SLOT(SK_FX, EXT_FX_P1); s[5] = SLOT(SK_FX, EXT_FX_P2); s[6] = SLOT(SK_FX, EXT_FX_P3); s[7] = SLOT(SK_FX, EXT_FX_P4);
        } else if (type == 5) {
            s[3] = SLOT(SK_FX, EXT_FX_REVERBTYPE);
            s[4] = SLOT(SK_FX, EXT_FX_P1); s[5] = SLOT(SK_FX, EXT_FX_P2); s[6] = SLOT(SK_FX, EXT_FX_P3); s[7] = SLOT(SK_FX, EXT_FX_P4);
        }
        break;
    }
    case M_LASTSTEP: s[0] = SLOT(SK_LAST, 0); break;
    case M_REC: s[0] = SLOT(SK_GLOB, G_TRK); s[1] = SLOT(SK_GLOB, G_QNT); break;
    case M_CLEAR: s[0] = SLOT(SK_CLEAR, 0); break;
    case M_LOAD: case M_SAVE:
        if (x->ui.lsType != 4) {
            s[0] = SLOT(SK_LSSLOT, 0);
            if (x->ui.mode == M_SAVE && x->ui.lsState == 2) {
                s[1] = SLOT(SK_NAMEPOS, 0); s[2] = SLOT(SK_NAMECASE, 0); s[3] = SLOT(SK_NAMECHR, 0);
            }
        }
        break;
    case M_SONG: s[0] = SLOT(SK_SONGROW, 0); s[1] = SLOT(SK_SONGPAT, 0); break;
    default: break;
    }
    if (!x->slots[x->ui.sel].kind) {
        for (i = 0; i < NUM_SLOTS; i++) if (x->slots[i].kind) { x->ui.sel = (uint8_t)i; break; }
    }
    x->displayRev++;
}

/* ---------------------------------------------------------------- slot values */
static uint8_t *kitb(Lxr *x) { return x->kit.b; }

static uint8_t lfo_range(Lxr *x, uint8_t lfo, uint8_t *start, uint8_t *end)
{
    uint8_t voice = kitb(x)[PAR_VOICE_LFO1 + lfo];
    if (voice < 1 || voice > 6) voice = 1;
    meta_modTargetRange((uint8_t)(voice - 1), start, end);
    return voice;
}

/* a mod target selection's slider: 0 = off, then the voice's targets in order */
static int target_to_pos(uint8_t value, uint8_t start, uint8_t end)
{
    if (value < start || value > end) return 0;
    return value - start + 1;
}

static int slot_max(Lxr *x, const Slot *s)
{
    uint8_t a, b;
    switch (s->kind) {
    case SK_SOUND: return meta_soundMax(s->id);
    case SK_EXT: return 127;
    case SK_FX:
        switch (s->id) {
        case EXT_FX_TYPE: return 5;
        case EXT_FX_OUT: return 5;
        case EXT_FX_DW: return 100;
        case EXT_FX_DRVTYPE: return 2;
        case EXT_FX_RINGWAVE: return 5;
        case EXT_FX_RATIO: return 5;
        case EXT_FX_DELAYTYPE: return 1;
        case EXT_FX_REVERBTYPE: return 0;
        default: return 127;
        }
    case SK_LFOVOICE: return 5;
    case SK_LFODST: lfo_range(x, s->id, &a, &b); return b - a + 1;
    case SK_VELDST: meta_modTargetRange(s->id, &a, &b); return b - a + 1;
    case SK_TRKLEN: return 63;
    case SK_TRKCH: return 15;
    case SK_TRKNOTE: return 127;
    case SK_STEP:
        switch (s->id) {
        case 2: return 100;
        case 3: case 5: { meta_modTargetRange(x->ui.track > 5 ? 5 : x->ui.track, &a, &b); return b - a + 1; }
        default: return 127;
        }
    case SK_GLOB:
        switch (s->id) {
        case G_BPM: return 255;
        case G_CH: return 15;
        case G_MTX: case G_MRX: return 15;
        case G_FLW: case G_KCM: case G_PAD: case G_LRN: case G_TRK: return 1;
        case G_ROLL: return 13;
        case G_SHU: return 127;
        case G_QNT: return 4;
        }
        return 1;
    case SK_MORPH: return 255;
    case SK_GEN: return s->id == 0 ? 63 : 64;
    case SK_LSSLOT: return 63;
    case SK_NAMEPOS: return NAME_LEN;   /* 0..7 the letters, 8 = ok */
    case SK_NAMECASE: return 1;
    case SK_NAMECHR: return 94;
    case SK_CLEAR: return 4;
    case SK_SONGROW: return SONG_LEN - 1;
    case SK_SONGPAT: return NUM_PATTERNS + 1;   /* p01..p64, END, LOOP */
    case SK_LAST: return 63;
    }
    return 1;
}

static Step *cur_step(Lxr *x) { return &x->pat.s[x->ui.track][x->ui.step]; }

static int slot_value(Lxr *x, const Slot *s)
{
    uint8_t a, b;
    switch (s->kind) {
    case SK_SOUND: return kitb(x)[s->id];
    case SK_EXT: case SK_FX: return kitb(x)[LXR_NUM_SOUND_PARAMS + s->id];
    case SK_LFOVOICE: { uint8_t v = kitb(x)[PAR_VOICE_LFO1 + s->id]; return (v < 1 || v > 6) ? 0 : v - 1; }
    case SK_LFODST: lfo_range(x, s->id, &a, &b); return target_to_pos(kitb(x)[PAR_TARGET_LFO1 + s->id], a, b);
    case SK_VELDST: meta_modTargetRange(s->id, &a, &b); return target_to_pos(kitb(x)[PAR_VEL_DEST_1 + s->id], a, b);
    case SK_TRKLEN: return (x->pat.len[x->ui.track] ? x->pat.len[x->ui.track] : 16) - 1;
    case SK_TRKCH: return x->p.g.trackCh[s->id];
    case SK_TRKNOTE: return x->p.g.trackNote[s->id];
    case SK_STEP: {
        Step *st = cur_step(x);
        meta_modTargetRange(x->ui.track > 5 ? 5 : x->ui.track, &a, &b);
        switch (s->id) {
        case 0: return st->vel;
        case 1: return st->note;
        case 2: return st->prob;
        case 3: return st->p1 ? target_to_pos(meta_paramToModTarget((uint16_t)(st->p1 - 1)), a, b) : 0;
        case 4: return st->v1;
        case 5: return st->p2 ? target_to_pos(meta_paramToModTarget((uint16_t)(st->p2 - 1)), a, b) : 0;
        default: return st->v2;
        }
    }
    case SK_GLOB:
        switch (s->id) {
        case G_BPM: return x->p.g.bpm;
        case G_CH: return x->p.g.midiCh;
        case G_MTX: return x->p.g.mtx;
        case G_MRX: return x->p.g.mrx;
        case G_FLW: return x->p.g.flw;
        case G_KCM: return x->p.g.kcm;
        case G_PAD: return x->padMode;
        case G_LRN: return 0;
        case G_ROLL: return x->p.g.roll;
        case G_SHU: return x->p.g.shuffle;
        case G_TRK: return x->p.g.recTrack;
        case G_QNT: return x->p.g.quant;
        }
        return 0;
    case SK_MORPH: return x->morph;
    case SK_GEN: {
        const uint8_t t = x->ui.track;
        return s->id == 0 ? x->ui.genLen[t] - 1 : s->id == 1 ? x->ui.genSteps[t] : x->ui.genRot[t];
    }
    case SK_LSSLOT: return x->ui.lsSlot;
    case SK_NAMEPOS: return x->ui.nameCursor;
    case SK_NAMECASE: {
        const char c = x->ui.nameCursor < NAME_LEN ? x->ui.name[x->ui.nameCursor] : 'a';
        return c >= 'a' && c <= 'z' ? 0 : 1;
    }
    case SK_NAMECHR: {
        const char c = x->ui.nameCursor < NAME_LEN ? x->ui.name[x->ui.nameCursor] : ' ';
        return c >= 32 && c < 127 ? c - 32 : 0;
    }
    case SK_CLEAR: return x->ui.clearSel;
    case SK_SONGROW: return x->ui.songRow;
    case SK_SONGPAT: {
        const uint8_t e = x->p.songs[x->songSlot].e[x->ui.songRow];
        return e < NUM_PATTERNS ? e : e == SONG_END ? NUM_PATTERNS : NUM_PATTERNS + 1;
    }
    case SK_LAST: return (x->pat.len[x->ui.track] ? x->pat.len[x->ui.track] : 16) - 1;
    }
    return 0;
}

static void set_lfo_voice(Lxr *x, uint8_t lfo, uint8_t voice1)
{
    /* keep the destination on the same parameter of the new voice where it has one */
    uint8_t a, b, a2, b2;
    const uint8_t oldTarget = kitb(x)[PAR_TARGET_LFO1 + lfo];
    int pos;
    lfo_range(x, lfo, &a, &b);
    pos = target_to_pos(oldTarget, a, b);
    lxr_setKitParam(x, (uint16_t)(PAR_VOICE_LFO1 + lfo), voice1, 0);
    meta_modTargetRange((uint8_t)(voice1 - 1), &a2, &b2);
    if (pos > b2 - a2 + 1) pos = b2 - a2 + 1;
    lxr_setKitParam(x, (uint16_t)(PAR_TARGET_LFO1 + lfo), (uint8_t)(pos ? a2 + pos - 1 : 0), 0);
}

static void lock_step_param(Lxr *x, uint16_t par, uint8_t value)
{
    /* firmware 1.70: a step held down shows and takes parameter changes; a step keeps two, the oldest is replaced */
    Step *st = &x->pat.s[x->ui.track][x->ui.heldStep];
    if (st->p1 == par + 1) st->v1 = value;
    else if (st->p2 == par + 1) st->v2 = value;
    else { st->p1 = st->p2; st->v1 = st->v2; st->p2 = (uint8_t)(par + 1); st->v2 = value; }
    x->ui.heldStepUsed = 1;
}

static void slot_set(Lxr *x, const Slot *s, int v)
{
    uint8_t a, b;
    const int mx = slot_max(x, s);
    if (v < 0) v = 0;
    if (v > mx) v = mx;
    switch (s->kind) {
    case SK_SOUND:
        if (x->ui.heldStep >= 0 && x->ui.mode == M_VOICE) { lock_step_param(x, s->id, (uint8_t)v); break; }
        lxr_setKitParam(x, s->id, (uint8_t)v, 1);
        break;
    case SK_EXT: case SK_FX: lxr_setExt(x, s->id, (uint8_t)v); if (s->id == EXT_FX_TYPE) ui_refreshSlots(x); break;
    case SK_LFOVOICE: set_lfo_voice(x, s->id, (uint8_t)(v + 1)); break;
    case SK_LFODST:
        lfo_range(x, s->id, &a, &b);
        lxr_setKitParam(x, (uint16_t)(PAR_TARGET_LFO1 + s->id), (uint8_t)(v ? a + v - 1 : 0), 0);
        break;
    case SK_VELDST:
        meta_modTargetRange(s->id, &a, &b);
        lxr_setKitParam(x, (uint16_t)(PAR_VEL_DEST_1 + s->id), (uint8_t)(v ? a + v - 1 : 0), 0);
        break;
    case SK_TRKLEN: case SK_LAST:
        if (x->ui.mode == M_LASTSTEP && x->ui.lastStepAll) { int t; for (t = 0; t < NUM_TRACKS; t++) x->pat.len[t] = (uint8_t)(v + 1); }
        else x->pat.len[x->ui.track] = (uint8_t)(v + 1);
        break;
    case SK_TRKCH: x->p.g.trackCh[s->id] = (uint8_t)v; break;
    case SK_TRKNOTE: x->p.g.trackNote[s->id] = (uint8_t)v; break;
    case SK_STEP: {
        Step *st = cur_step(x);
        meta_modTargetRange(x->ui.track > 5 ? 5 : x->ui.track, &a, &b);
        switch (s->id) {
        case 0: st->vel = (uint8_t)v; break;
        case 1: st->note = (uint8_t)v; break;
        case 2: st->prob = (uint8_t)v; break;
        case 3: st->p1 = v ? (uint8_t)(meta_modTargetParam((uint8_t)(a + v - 1)) + 1) : 0; break;
        case 4: st->v1 = (uint8_t)v; break;
        case 5: st->p2 = v ? (uint8_t)(meta_modTargetParam((uint8_t)(a + v - 1)) + 1) : 0; break;
        default: st->v2 = (uint8_t)v; break;
        }
        break;
    }
    case SK_GLOB:
        switch (s->id) {
        case G_BPM:
            if ((x->p.g.bpm == 0) != (v == 0)) seq_stop(x);   /* switching between MPC and the LXR's own clock */
            x->p.g.bpm = (uint8_t)v;
            break;
        case G_CH: x->p.g.midiCh = (uint8_t)v; break;
        case G_MTX: x->p.g.mtx = (uint8_t)v; break;
        case G_MRX: x->p.g.mrx = (uint8_t)v; break;
        case G_FLW: x->p.g.flw = (uint8_t)v; break;
        case G_KCM: x->p.g.kcm = (uint8_t)v; break;
        case G_PAD: x->padMode = (uint8_t)v; store_saveSettings(x); break;
        case G_LRN:
            if (v) { x->ui.prevMode = x->ui.mode; x->ui.mode = M_PADLEARN; x->ui.learnStage = 0; ui_refreshSlots(x); }
            break;
        case G_ROLL: x->p.g.roll = (uint8_t)v; break;
        case G_SHU: x->p.g.shuffle = (uint8_t)v; break;
        case G_TRK: x->p.g.recTrack = (uint8_t)v; break;
        case G_QNT: x->p.g.quant = (uint8_t)v; break;
        }
        break;
    case SK_MORPH:
        x->morph = (uint8_t)v;
        lxr_applyKit(x);
        break;
    case SK_GEN: {
        const uint8_t t = x->ui.track;
        if (s->id == 0) x->ui.genLen[t] = (uint8_t)(v + 1);
        else if (s->id == 1) x->ui.genSteps[t] = (uint8_t)v;
        else x->ui.genRot[t] = (uint8_t)v;
        if (x->ui.genSteps[t] > x->ui.genLen[t]) x->ui.genSteps[t] = x->ui.genLen[t];
        seq_euclid(x, t);   /* every change of length, steps or rotation generates the pattern anew (7.5) */
        break;
    }
    case SK_LSSLOT: x->ui.lsSlot = (uint8_t)v; x->ui.lsState = 0; break;
    case SK_NAMEPOS: x->ui.nameCursor = (uint8_t)v; break;
    case SK_NAMECASE:
        if (x->ui.nameCursor < NAME_LEN) {
            char *c = &x->ui.name[x->ui.nameCursor];
            if (v == 0 && *c >= 'A' && *c <= 'Z') *c = (char)(*c + 32);
            if (v == 1 && *c >= 'a' && *c <= 'z') *c = (char)(*c - 32);
        }
        break;
    case SK_NAMECHR: if (x->ui.nameCursor < NAME_LEN) x->ui.name[x->ui.nameCursor] = (char)(32 + v); break;
    case SK_CLEAR: x->ui.clearSel = (uint8_t)v; break;
    case SK_SONGROW: x->ui.songRow = (uint8_t)v; break;
    case SK_SONGPAT:
        x->p.songs[x->songSlot].e[x->ui.songRow] = (uint8_t)(v < NUM_PATTERNS ? v : v == NUM_PATTERNS ? SONG_END : SONG_LOOP);
        break;
    }
    x->displayRev++;
}

static const char *slot_short(Lxr *x, const Slot *s, char *buf)
{
    static const char *const stepNames[7] = {"vel", "nte", "prb", "p1d", "p1v", "p2d", "p2v"};
    static const char *const globNames[12] = {"bpm", "ch", "mtx", "mrx", "flw", "kcm", "pad", "lrn", "rol", "shu", "trk", "qnt"};
    static const char *const genNames[3] = {"len", "stp", "rot"};
    switch (s->kind) {
    case SK_SOUND: {
        /* the name the menu page gives it; parameters on two pages share a name */
        const uint8_t mt = meta_paramToModTarget(s->id);
        if (s->id == PAR_VOICE_DECIMATION_ALL) return "sr";
        if (mt) { char t[8]; meta_modTargetText(mt, t, 0); strcpy(buf, t); return buf; }
        return "";
    }
    case SK_EXT: return "pwm";
    case SK_FX: {
        const uint8_t type = x->kit.b[LXR_NUM_SOUND_PARAMS + EXT_FX_TYPE];
        switch (s->id) {
        case EXT_FX_TYPE: return "typ";
        case EXT_FX_OUT: return "out";
        case EXT_FX_DW: return "d/w";
        case EXT_FX_DRVTYPE: case EXT_FX_DELAYTYPE: case EXT_FX_REVERBTYPE: return "typ";
        case EXT_FX_RINGWAVE: return "wav";
        case EXT_FX_RATIO: return "rat";
        case EXT_FX_P1: return type == 1 ? "drv" : type == 2 ? "frq" : type == 3 ? "atk" : type == 5 ? "pre" : "tim";
        case EXT_FX_P2: return type == 1 ? "col" : type == 3 || type == 5 ? "dec" : "rng";
        case EXT_FX_P3: return type == 1 ? "ton" : type == 3 ? "tre" : "ton";
        case EXT_FX_P4: return type == 1 ? "vol" : type == 3 ? "gai" : type == 5 ? "ref" : "fbk";
        }
        return "";
    }
    case SK_LFOVOICE: return "voi";
    case SK_LFODST: case SK_VELDST: return "dst";
    case SK_TRKLEN: return "len";
    case SK_TRKCH: return "ch";
    case SK_TRKNOTE: return "nte";
    case SK_STEP: return stepNames[s->id];
    case SK_GLOB: return globNames[s->id];
    case SK_MORPH: return "mrp";
    case SK_GEN: return genNames[s->id];
    case SK_LSSLOT: return "slt";
    case SK_NAMEPOS: return "pos";
    case SK_NAMECASE: return "abc";
    case SK_NAMECHR: return "chr";
    case SK_CLEAR: return "clr";
    case SK_SONGROW: return "row";
    case SK_SONGPAT: return "pat";
    case SK_LAST: return "lst";
    }
    return "";
}

static const char *slot_long(Lxr *x, const Slot *s)
{
    switch (s->kind) {
    case SK_SOUND: {
        const uint8_t mt = meta_paramToModTarget(s->id);
        extern const char *meta_targetLong(uint8_t idx);
        if (s->id == PAR_VOICE_DECIMATION_ALL) return "SampleRt";
        return mt ? meta_targetLong(mt) : "";
    }
    case SK_EXT: return "PulsWdth";
    case SK_FX: {
        const uint8_t type = x->kit.b[LXR_NUM_SOUND_PARAMS + EXT_FX_TYPE];
        switch (s->id) {
        case EXT_FX_TYPE: return "FX Type";
        case EXT_FX_OUT: return "AudioOut";
        case EXT_FX_DW: return "Dry/Wet";
        case EXT_FX_DRVTYPE: return "Drive Type";
        case EXT_FX_DELAYTYPE: return "Delay Type";
        case EXT_FX_REVERBTYPE: return "Reverb Type";
        case EXT_FX_RINGWAVE: return "Waveform";
        case EXT_FX_RATIO: return "Ratio";
        case EXT_FX_P1: return type == 1 ? "Overdriv" : type == 2 ? "Frequncy" : type == 3 ? "Attack" : type == 5 ? "PreDelay" : "Time";
        case EXT_FX_P2: return type == 1 ? "Color" : type == 3 || type == 5 ? "Decay" : "Range";
        case EXT_FX_P3: return type == 1 ? "Tone" : type == 3 ? "Threshld" : "Tone";
        case EXT_FX_P4: return type == 1 ? "Volume" : type == 3 ? "Gain" : type == 5 ? "Reflxion" : "Feedback";
        }
        return "";
    }
    case SK_LFOVOICE: return "DstVoice";
    case SK_LFODST: case SK_VELDST: return "DstParam";
    case SK_TRKLEN: case SK_LAST: return "Length";
    case SK_TRKCH: return "Channel";
    case SK_TRKNOTE: return "Note";
    case SK_STEP: { static const char *const n[7] = {"Velocity", "Note", "Prbablty", "Aut Dest", "AutValue", "Aut Dest", "AutValue"}; return n[s->id]; }
    case SK_GLOB: {
        static const char *const n[12] = {"Tempo", "Channel", "Transmit", "Receive", "Follow", "KitChMde", "Pads", "Pad learn",
                                           "RollRate", "Shuffle", "AutTrack", "Quantize"};
        return n[s->id];
    }
    case SK_MORPH: return "Morph";
    case SK_GEN: { static const char *const n[3] = {"Length", "Steps", "Rotation"}; return n[s->id]; }
    case SK_LSSLOT: return "Slot";
    case SK_NAMEPOS: return "Cursor";
    case SK_NAMECASE: return "Case";
    case SK_NAMECHR: return "Letter";
    case SK_CLEAR: return "Clear";
    case SK_SONGROW: return "Row";
    case SK_SONGPAT: return "Pattern";
    }
    return "";
}

static void slot_text(Lxr *x, const Slot *s, char *buf)
{
    const int v = slot_value(x, s);
    uint8_t a, b;
    switch (s->kind) {
    case SK_SOUND: meta_soundText(s->id, (uint8_t)v, buf); return;
    case SK_EXT: sprintf(buf, "%d", v); return;
    case SK_FX:
        switch (s->id) {
        case EXT_FX_TYPE: strcpy(buf, fxTypes[v < 6 ? v : 0]); return;
        case EXT_FX_OUT: strcpy(buf, outNames[v < 6 ? v : 0]); return;
        case EXT_FX_DRVTYPE: strcpy(buf, drvTypes[v < 3 ? v : 0]); return;
        case EXT_FX_RINGWAVE: strcpy(buf, ringWaves[v < 6 ? v : 0]); return;
        case EXT_FX_REVERBTYPE: strcpy(buf, "Plt"); return;
        case EXT_FX_RATIO: strcpy(buf, ratioNames[v < 6 ? v : 0]); return;
        case EXT_FX_DELAYTYPE: strcpy(buf, delayTypes[v < 2 ? v : 0]); return;
        }
        sprintf(buf, "%d", v);
        return;
    case SK_LFOVOICE: sprintf(buf, "v%d", v + 1); return;
    case SK_LFODST: lfo_range(x, s->id, &a, &b); meta_modTargetText((uint8_t)(v ? a + v - 1 : 0), buf, 0); return;
    case SK_VELDST: meta_modTargetRange(s->id, &a, &b); meta_modTargetText((uint8_t)(v ? a + v - 1 : 0), buf, 0); return;
    case SK_TRKLEN: case SK_LAST: case SK_GEN: sprintf(buf, "%d", s->kind == SK_GEN && s->id ? v : v + 1); return;
    case SK_TRKCH: sprintf(buf, "%d", v + 1); return;
    case SK_TRKNOTE: if (v == 0) strcpy(buf, "any"); else meta_noteName((uint8_t)v, buf); return;
    case SK_STEP:
        if (s->id == 1) { meta_noteName((uint8_t)v, buf); return; }
        if (s->id == 2) { sprintf(buf, "%d%%", v); return; }
        if (s->id == 3 || s->id == 5) {
            meta_modTargetRange(x->ui.track > 5 ? 5 : x->ui.track, &a, &b);
            meta_modTargetText((uint8_t)(v ? a + v - 1 : 0), buf, 0);
            return;
        }
        sprintf(buf, "%d", v);
        return;
    case SK_GLOB:
        switch (s->id) {
        case G_BPM: if (v == 0) strcpy(buf, "MPC"); else sprintf(buf, "%d", v); return;
        case G_CH: sprintf(buf, "%d", v + 1); return;
        case G_MTX: case G_MRX: strcpy(buf, filtNames[v & 15]); return;
        case G_FLW: strcpy(buf, v ? "on" : "off"); return;
        case G_KCM: strcpy(buf, v ? "Aut" : "off"); return;
        case G_PAD: strcpy(buf, v ? "nte" : "pnl"); return;
        case G_LRN: strcpy(buf, "go"); return;
        case G_ROLL: strcpy(buf, rollNames[v < 14 ? v : 8]); return;
        case G_TRK: sprintf(buf, "%d", v + 1); return;
        case G_QNT: strcpy(buf, quantNames[v < 5 ? v : 2]); return;
        }
        sprintf(buf, "%d", v);
        return;
    case SK_LSSLOT: sprintf(buf, "%d", v + 1); return;
    case SK_NAMEPOS: if (v >= NAME_LEN) strcpy(buf, "ok"); else sprintf(buf, "%d", v + 1); return;
    case SK_NAMECASE: strcpy(buf, v ? "ABC" : "abc"); return;
    case SK_NAMECHR: sprintf(buf, "%c", 32 + v); return;
    case SK_CLEAR: strcpy(buf, clearNames[v < 5 ? v : 0]); return;
    case SK_SONGROW: sprintf(buf, "%03d", v); return;
    case SK_SONGPAT: if (v < NUM_PATTERNS) sprintf(buf, "p%02d", v + 1); else strcpy(buf, v == NUM_PATTERNS ? "END" : "LOOP"); return;
    }
    sprintf(buf, "%d", v);
}

/* ---- the screen sliders (engine.c): value 0..1, name, value text, shown or not ---- */
float ui_slotGet(Lxr *x, int i)
{
    const Slot *s = &x->slots[i];
    int mx;
    if (!s->kind) return 0;
    mx = slot_max(x, s);
    return mx > 0 ? (float)slot_value(x, s) / (float)mx : 0;
}

void ui_slotSet(Lxr *x, int i, float n)
{
    const Slot *s = &x->slots[i];
    int mx, cur, v;
    float pos;
    if (!s->kind) return;
    mx = slot_max(x, s);
    cur = slot_value(x, s);
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    pos = n * mx;
    /* as the plugin wrapper settles option lists: a value on a step is taken as is (a tap, a jump); a small move from
     * the value read back (a Q-Link event, a data wheel click) is one step in its direction */
    if (pos - (float)(int)(pos + 0.5f) > -0.002f && pos - (float)(int)(pos + 0.5f) < 0.002f) v = (int)(pos + 0.5f);
    else if (pos > cur && pos - cur < 1.f) v = cur + 1;
    else if (pos < cur && cur - pos < 1.f) v = cur - 1;
    else v = (int)(pos + 0.5f);
    x->ui.sel = (uint8_t)i;
    x->ui.potHalf = (uint8_t)(i / 4);
    if (v != cur) slot_set(x, s, v);
    x->displayRev++;
}

const char *ui_slotName(Lxr *x, int i, char *buf)
{
    const Slot *s = &x->slots[i];
    char t[16];
    const char *n;
    if (!s->kind) { buf[0] = 0; return buf; }
    n = slot_short(x, s, t);
    snprintf(buf, 16, "%s", n);
    if (i == x->ui.sel) {   /* the selected parameter in capitals, as on the LCD */
        char *c;
        for (c = buf; *c; c++) if (*c >= 'a' && *c <= 'z') *c = (char)(*c - 32);
    }
    return buf;
}

const char *ui_slotDisplay(Lxr *x, int i, char *buf)
{
    if (!x->slots[i].kind) { buf[0] = 0; return buf; }
    slot_text(x, &x->slots[i], buf);
    return buf;
}

int ui_slotUsed(Lxr *x, int i) { return x->slots[i].kind != SK_NONE; }

/* ---------------------------------------------------------------- modes */
static void enter(Lxr *x, uint8_t mode)
{
    if (x->ui.mode != M_LOAD && x->ui.mode != M_SAVE && x->ui.mode != M_CLEAR && x->ui.mode != M_PADLEARN)
        x->ui.prevMode = x->ui.mode;
    x->ui.mode = mode;
    x->ui.lsArm = 0;
    x->ui.sel = 0;
    x->ui.potHalf = 0;
    ui_refreshSlots(x);
}

void ui_message(Lxr *x, const char *l1, const char *l2, int ms)
{
    snprintf(x->ui.msg[0], sizeof x->ui.msg[0], "%s", l1);
    snprintf(x->ui.msg[1], sizeof x->ui.msg[1], "%s", l2);
    x->ui.msgUntil = lxr_clock(x) + (uint32_t)(ms * 44.1f);
    x->displayRev++;
}

static int shifted(Lxr *x) { return x->ui.shift || x->ui.shiftHeld; }
static void shift_done(Lxr *x) { x->ui.shift = 0; }   /* a menu reached through a latched SHIFT: the latch is spent */

static void ls_name_from_source(Lxr *x)
{
    const char *src = "Unnamed ";
    char tmp[NAME_LEN + 1];
    switch (x->ui.lsType) {
    case 0: src = x->kit.name; break;
    case 1: src = x->kit.name; break;
    case 2: src = x->p.songs[x->songSlot].name; break;
    case 3: src = x->p.name; break;
    }
    memcpy(tmp, src, NAME_LEN);
    tmp[NAME_LEN] = 0;
    if (!tmp[0]) memcpy(tmp, "Unnamed ", NAME_LEN);
    memcpy(x->ui.name, tmp, NAME_LEN);
}

static void enter_ls(Lxr *x, uint8_t save, uint8_t type)
{
    x->ui.lsType = type;
    x->ui.lsState = 0;
    x->ui.nameCursor = 0;
    switch (type) {
    case 0: case 1: x->ui.lsSlot = x->kitSlot; break;
    case 2: x->ui.lsSlot = x->songSlot; break;
    case 3: x->ui.lsSlot = x->projSlot; break;
    default: x->ui.group = (uint8_t)(x->patSlot / 16); break;
    }
    enter(x, save ? M_SAVE : M_LOAD);
}

static void back(Lxr *x)
{
    uint8_t m = x->ui.prevMode;
    if (m == M_LOAD || m == M_SAVE || m == M_CLEAR || m == M_PADLEARN) m = M_VOICE;
    x->ui.mode = m;
    x->ui.lsArm = 0;
    ui_refreshSlots(x);
}

static void slot_name(Lxr *x, const char *src, char *dst)
{
    int i;
    (void)x;
    for (i = 0; i < NAME_LEN; i++) dst[i] = src[i] >= 32 && src[i] < 127 ? src[i] : ' ';
    dst[NAME_LEN] = 0;
}

/* the step key that picks each load/save type (7.1: KIT MRP SNG PRJ, then PAT on step 16) */
static int ls_key(uint8_t type) { return type < 4 ? 7 + type : 15; }

static void ls_confirm(Lxr *x)
{
    const uint8_t slot = x->ui.lsSlot;
    if (x->ui.mode == M_LOAD) {
        switch (x->ui.lsType) {
        case 0:
            x->kit = x->p.kits[slot];
            x->kitSlot = slot;
            lxr_applyKit(x);
            ui_message(x, "Load: Kit", "loaded", 700);
            break;
        case 1:
            x->morphKit = x->p.kits[slot];
            x->morphLoaded = 1;
            lxr_applyKit(x);
            ui_message(x, "Load: MorphKit", "loaded", 700);
            break;
        case 2:
            x->songSlot = slot;
            ui_message(x, "Load: Song", "loaded", 700);
            break;
        case 3:
            if (store_loadProject(x, slot)) ui_message(x, "Loading Project", "", 1000);
            else ui_message(x, "Load: Project", "File not found!", 1200);
            break;
        }
        back(x);
        return;
    }
    /* SAVE: slot -> "Edit Name? [y/N]" -> (name) -> save */
    if (x->ui.lsState == 0) {
        x->ui.lsState = 1;
        x->ui.lsYes = 0;
        ls_name_from_source(x);
        x->displayRev++;
        return;
    }
    if (x->ui.lsState == 1 && x->ui.lsYes) {
        x->ui.lsState = 2;
        x->ui.nameCursor = 0;
        ui_refreshSlots(x);
        return;
    }
    if (x->ui.lsState == 2 && x->ui.nameCursor < NAME_LEN) return;   /* push on "ok" to save */
    switch (x->ui.lsType) {
    case 0:
        memcpy(x->kit.name, x->ui.name, NAME_LEN);
        x->p.kits[slot] = x->kit;
        x->kitSlot = slot;
        ui_message(x, "Save Kit", "success!", 800);
        break;
    case 1: {
        /* the morph result as a new kit (7.4 "Saving" a morph kit) */
        Kit k = x->kit;
        int i;
        if (x->morphLoaded)
            for (i = 0; i < LXR_NUM_SOUND_PARAMS; i++) k.b[i] = lxr_dsp_morphValue(x->kit.b, x->morphKit.b, (uint16_t)i, x->morph);
        memcpy(k.name, x->ui.name, NAME_LEN);
        x->p.kits[slot] = k;
        ui_message(x, "Save MorphKit", "success!", 800);
        break;
    }
    case 2:
        memcpy(x->p.songs[x->songSlot].name, x->ui.name, NAME_LEN);
        if (slot != x->songSlot) x->p.songs[slot] = x->p.songs[x->songSlot];
        x->songSlot = slot;
        ui_message(x, "Save Song", "success!", 800);
        break;
    case 3:
        memcpy(x->p.name, x->ui.name, NAME_LEN);
        x->projSlot = slot;
        ui_message(x, "Saving Project", store_saveProject(x, slot) ? "" : "SD Card error:", 1200);
        break;
    }
    back(x);
}

static void do_clear(Lxr *x)
{
    int s;
    const uint8_t t = x->ui.track;
    switch (x->ui.clearSel) {
    case 0: lxr_initKit(&x->kit); lxr_applyKit(x); break;
    case 1: for (s = 0; s < NUM_STEPS; s++) { Step *st = &x->pat.s[t][s]; memset(st, 0, sizeof *st); st->vel = 100; st->note = 63; st->prob = 100; } break;
    case 2: { uint8_t k = x->pat.kit; lxr_initPattern(&x->pat); x->pat.kit = k; break; }
    case 3: for (s = 0; s < NUM_STEPS; s++) { x->pat.s[t][s].p1 = 0; x->pat.s[t][s].v1 = 0; } break;
    default: for (s = 0; s < NUM_STEPS; s++) { x->pat.s[t][s].p2 = 0; x->pat.s[t][s].v2 = 0; } break;
    }
    ui_message(x, "clear", clearNames[x->ui.clearSel], 700);
    x->ui.shift = 0;
    back(x);
}

static void toggle_step(Lxr *x, uint8_t s)
{
    Step *st = &x->pat.s[x->ui.track][s];
    st->on = (uint8_t)!st->on;
    if (st->on) {
        if (!st->vel) st->vel = 100;
        if (!st->note) st->note = 63;
        if (!st->prob) st->prob = 100;
    }
    x->displayRev++;
}

static void copy_track(Lxr *x, uint8_t src, uint8_t dst)
{
    memcpy(x->pat.s[dst], x->pat.s[src], sizeof x->pat.s[0]);
    x->pat.len[dst] = x->pat.len[src];
}

static void copy_bar(Lxr *x)
{
    /* COPY + BAR: fill the next bar with this one, extending the track (6.1 Track length) */
    const uint8_t t = x->ui.track, b = x->ui.bar;
    int i;
    if (b >= 3) return;
    for (i = 0; i < 16; i++) x->pat.s[t][(b + 1) * 16 + i] = x->pat.s[t][b * 16 + i];
    if (x->pat.len[t] < (b + 2) * 16) x->pat.len[t] = (uint8_t)((b + 2) * 16);
    x->ui.bar = (uint8_t)(b + 1);
    ui_message(x, "copy bar", "", 600);
}

static void select_pattern_key(Lxr *x, uint8_t k)
{
    const uint8_t pat = (uint8_t)(x->ui.group * 16 + k);
    if (x->ui.mode == M_SAVE) {
        x->pat.kit = x->kitSlot;
        x->p.pats[pat] = x->pat;
        x->patSlot = pat;
        ui_message(x, "Save Pattern", "success!", 800);
        back(x);
        return;
    }
    seq_queuePattern(x, pat);
    if (x->ui.mode == M_LOAD) back(x);
}

void ui_press(Lxr *x, int b, int fromPad)
{
    const int sh = shifted(x);
    if (b < 0 || b >= NUM_BUTTONS) return;
    x->ui.pressedAt[b] = lxr_clock(x);
    if (fromPad) x->ui.held[b] = 1;
    if (x->ui.mode == M_PADLEARN && b != B_ENC) return;

    if (b == B_SHIFT) {
        if (fromPad) x->ui.shiftHeld = 1;
        else {
            x->ui.shift = (uint8_t)!x->ui.shift;
            if (!x->ui.shift && x->ui.mode == M_CLEAR) back(x);   /* releasing SHIFT aborts the clear menu */
        }
        x->displayRev++;
        return;
    }

    if (b == B_COPY) {
        if (x->ui.mode == M_CLEAR) { do_clear(x); return; }
        if (sh && x->ui.mode == M_SONG) {
            /* delete the row */
            Song *s = &x->p.songs[x->songSlot];
            memmove(&s->e[x->ui.songRow], &s->e[x->ui.songRow + 1], SONG_LEN - 1 - x->ui.songRow);
            s->e[SONG_LEN - 1] = SONG_END;
            x->displayRev++;
            return;
        }
        if (sh && x->seq.recording && x->seq.running) {
            /* SHIFT + CLEAR while recording: erase at the playhead (held on a pad; toggled on the screen) */
            x->seq.erasing = fromPad ? 1 : (uint8_t)!x->seq.erasing;
            return;
        }
        if (sh) { x->ui.clearSel = 1; enter(x, M_CLEAR); return; }
        if (x->ui.mode == M_SONG) {
            /* insert a copy of the row below */
            Song *s = &x->p.songs[x->songSlot];
            if (x->ui.songRow < SONG_LEN - 1) {
                memmove(&s->e[x->ui.songRow + 1], &s->e[x->ui.songRow], SONG_LEN - 1 - x->ui.songRow - 1 + 1);
                x->ui.songRow++;
            }
            x->displayRev++;
            return;
        }
        if (fromPad) { x->ui.copyHeld = 1; x->ui.copy = 1; }
        else x->ui.copy = (uint8_t)!x->ui.copy;
        x->ui.copySrc = 0xff;
        x->displayRev++;
        return;
    }

    if (b == B_PLAY) {
        if (x->p.g.bpm == 0 && x->seq.hostPlaying) {   /* MPC is playing and the LXR follows it */
            if (sh) { x->seq.songMode = 1; shift_done(x); ui_message(x, "Song mode", "from MPC's next play", 900); }
            else ui_message(x, "Tempo: MPC", "MPC starts/stops", 900);
            return;
        }
        /* MPC stopped (or the LXR's own tempo): PLAY runs the LXR itself, at MPC's tempo when following it */
        if (sh) { if (x->seq.running) seq_stop(x); seq_start(x, 1); shift_done(x); return; }
        if (x->seq.running) seq_stop(x); else seq_start(x, 0);
        return;
    }

    if (b == B_REC) {
        if (sh) { enter(x, M_REC); shift_done(x); return; }
        x->seq.recording = (uint8_t)!x->seq.recording;
        if (!x->seq.recording) x->seq.erasing = 0;
        x->displayRev++;
        return;
    }

    if (b == B_VOICE) {
        if (x->ui.mode == M_VOICE) enter(x, M_STEP);
        else enter(x, M_VOICE);
        x->ui.copy = 0;
        return;
    }
    if (b == B_PERF) {
        if (sh) { enter(x, M_PATGEN); shift_done(x); return; }
        enter(x, M_PERF);
        return;
    }
    if (b == B_LOAD || b == B_SAVE) {
        const uint8_t which = b == B_LOAD ? 1 : 2;
        if ((x->ui.mode == M_LOAD && which == 1) || (x->ui.mode == M_SAVE && which == 2)) { back(x); return; }
        x->ui.lsArm = x->ui.lsArm == which ? 0 : which;   /* then KIT, MORPH, SONG, PROJECT or PATTERN */
        x->displayRev++;
        return;
    }
    if (b >= B_SEL1 && b <= B_SEL8) {
        const uint8_t sec = (uint8_t)(b - B_SEL1);
        if (x->ui.mode != M_VOICE) { x->ui.section = sec; enter(x, M_VOICE); return; }
        if (x->ui.section == sec) {
            /* pressing the menu button again toggles the menu's two pages (2.3): which four the pots edit */
            x->ui.potHalf ^= 1;
            x->ui.sel = (uint8_t)(x->ui.potHalf * 4);
            if (!x->slots[x->ui.sel].kind) { x->ui.potHalf = 0; x->ui.sel = 0; }
        } else {
            x->ui.section = sec;
            x->ui.sel = 0;
            x->ui.potHalf = 0;
        }
        ui_refreshSlots(x);
        return;
    }
    if (b == B_BARL || b == B_BARR) {
        const int dir = b == B_BARR ? 1 : -1;
        const uint8_t m = x->ui.mode;
        if (m == M_PATTERN || m == M_SONG || ((m == M_LOAD || m == M_SAVE) && x->ui.lsType == 4)) {
            int g = x->ui.group + dir;
            x->ui.group = (uint8_t)(g < 0 ? 0 : g > 3 ? 3 : g);
            x->displayRev++;
            return;
        }
        if (x->ui.copy && b == B_BARR) { copy_bar(x); x->ui.copy = 0; return; }
        if (sh && (m == M_VOICE || m == M_STEP)) {
            /* SHIFT + BAR: the track grows or shrinks by a bar */
            int len = (x->pat.len[x->ui.track] ? x->pat.len[x->ui.track] : 16) + 16 * dir;
            x->pat.len[x->ui.track] = (uint8_t)(len < 1 ? 1 : len > 64 ? 64 : len);
            ui_message(x, "Track length", "", 500);
            x->displayRev++;
            return;
        }
        {
            int nb = x->ui.bar + dir;
            x->ui.bar = (uint8_t)(nb < 0 ? 0 : nb > 3 ? 3 : nb);
        }
        x->displayRev++;
        return;
    }
    if (b >= B_V1 && b <= B_V7) {
        const uint8_t t = (uint8_t)(b - B_V1);
        if (x->ui.copy) {
            if (x->ui.copySrc == 0xff) { x->ui.copySrc = t; x->displayRev++; return; }
            copy_track(x, x->ui.copySrc, t);
            ui_message(x, "copy", "track copied", 600);
            x->ui.copy = 0;
            x->ui.copySrc = 0xff;
            return;
        }
        if (x->ui.mode == M_PERF) {
            /* performance mode: the voice keys mute without SHIFT; SHIFT + voice unmutes everything up to it */
            if (sh) x->seq.mutes &= (uint8_t)~((2u << t) - 1u);
            else x->seq.mutes ^= (uint8_t)(1u << t);
            x->displayRev++;
            return;
        }
        if (sh) { x->seq.mutes ^= (uint8_t)(1u << t); x->displayRev++; return; }
        if (x->ui.mode == M_LASTSTEP) x->ui.lastStepAll = 0;
        x->ui.track = t;
        ui_refreshSlots(x);
        return;
    }
    if (b >= B_S1 && b <= B_S16) {
        const uint8_t k = (uint8_t)(b - B_S1);
        if (x->ui.lsArm) {
            const uint8_t save = x->ui.lsArm == 2;
            switch (k) {
            case 7: enter_ls(x, save, 0); break;
            case 8: enter_ls(x, save, 1); break;
            case 9: enter_ls(x, save, 2); break;
            case 10: enter_ls(x, save, 3); break;
            case 15: enter_ls(x, save, 4); break;
            default: break;
            }
            return;
        }
        if (sh && k >= 9) {
            switch (k) {
            case 9: shift_done(x); enter(x, M_SONG); x->ui.group = 0; return;
            case 11: shift_done(x); enter(x, M_CONFIG); return;
            case 12:
                shift_done(x);
                x->kit = x->p.kits[x->kitSlot];
                lxr_applyKit(x);
                memcpy(&x->pat, &x->p.pats[x->patSlot], sizeof x->pat);
                ui_message(x, "Reload", "kit + pattern", 700);
                return;
            case 13:
                shift_done(x);
                if (x->ui.mode == M_LASTSTEP) x->ui.lastStepAll = (uint8_t)!x->ui.lastStepAll;   /* double press: all */
                else { x->ui.lastStepAll = 0; x->ui.bar = 0; enter(x, M_LASTSTEP); }
                x->displayRev++;
                return;
            case 14: shift_done(x); enter(x, M_FX); return;
            case 15: shift_done(x); x->ui.group = (uint8_t)(x->patSlot / 16); enter(x, M_PATTERN); return;
            default: return;
            }
        }
        switch (x->ui.mode) {
        case M_VOICE: case M_PATGEN:
            if (fromPad) { x->ui.heldStep = (int8_t)(x->ui.bar * 16 + k); x->ui.heldStepUsed = 0; }
            else toggle_step(x, (uint8_t)(x->ui.bar * 16 + k));
            return;
        case M_STEP:
            x->ui.step = (uint8_t)(x->ui.bar * 16 + k);
            x->displayRev++;
            return;
        case M_PERF:
            if (k < NUM_TRACKS) {
                if (x->p.g.roll == 0) { lxr_trigger(x, k, 100, 63, 1); return; }   /* "One": a single hit */
                if (fromPad) x->seq.rolls |= (uint8_t)(1u << k);
                else x->seq.rolls ^= (uint8_t)(1u << k);
                x->displayRev++;
            }
            return;
        case M_PATTERN: case M_LOAD: case M_SAVE:
            if (x->ui.lsType == 4 || x->ui.mode == M_PATTERN) select_pattern_key(x, k);
            else if (k == ls_key(x->ui.lsType)) ls_confirm(x);   /* the kit/morph/song/project key again: the push */
            return;
        case M_SONG:
            x->p.songs[x->songSlot].e[x->ui.songRow] = (uint8_t)(x->ui.group * 16 + k);
            x->displayRev++;
            return;
        case M_LASTSTEP: {
            const uint8_t len = (uint8_t)(x->ui.bar * 16 + k + 1);
            int t;
            if (x->ui.lastStepAll) for (t = 0; t < NUM_TRACKS; t++) x->pat.len[t] = len;
            else x->pat.len[x->ui.track] = len;
            x->displayRev++;
            return;
        }
        default: return;
        }
    }
    if (b == B_ENC) {
        if (x->ui.mode == M_PADLEARN) { x->ui.mode = x->ui.prevMode; ui_message(x, "Pad learn", "cancelled", 700); ui_refreshSlots(x); return; }
        if (x->ui.mode == M_LOAD || x->ui.mode == M_SAVE) { if (x->ui.lsType != 4) ls_confirm(x); return; }
        if (x->ui.mode == M_CLEAR) { do_clear(x); return; }
        if (x->ui.mode == M_SONG) { x->ui.sel = x->ui.sel == 0 ? 1 : 0; x->displayRev++; return; }
        if (x->ui.mode == M_CONFIG && x->slots[x->ui.sel].kind == SK_GLOB && x->slots[x->ui.sel].id == G_LRN) {
            slot_set(x, &x->slots[x->ui.sel], 1);
            return;
        }
        return;
    }
}

void ui_release(Lxr *x, int b, int fromPad)
{
    if (b < 0 || b >= NUM_BUTTONS) return;
    if (!fromPad) return;
    x->ui.held[b] = 0;
    if (b == B_SHIFT) {
        x->ui.shiftHeld = 0;
        if (x->ui.mode == M_CLEAR && !x->ui.shift) back(x);   /* releasing SHIFT aborts */
        x->seq.erasing = 0;
        x->displayRev++;
        return;
    }
    if (b == B_COPY) {
        x->ui.copyHeld = 0;
        x->ui.copy = 0;
        x->ui.copySrc = 0xff;
        x->seq.erasing = 0;
        x->displayRev++;
        return;
    }
    if (b >= B_S1 && b <= B_S16) {
        const uint8_t k = (uint8_t)(b - B_S1);
        if ((x->ui.mode == M_VOICE || x->ui.mode == M_PATGEN) && x->ui.heldStep == (int8_t)(x->ui.bar * 16 + k)) {
            if (!x->ui.heldStepUsed) toggle_step(x, (uint8_t)x->ui.heldStep);
            x->ui.heldStep = -1;
            x->displayRev++;
        }
        if (x->ui.mode == M_PERF && k < NUM_TRACKS) { x->seq.rolls &= (uint8_t)~(1u << k); x->displayRev++; }
    }
}

/* ---------------------------------------------------------------- encoder and pots */
void ui_data(Lxr *x, int steps)
{
    if (!steps) return;
    switch (x->ui.mode) {
    case M_LOAD: case M_SAVE:
        if (x->ui.lsType == 4) return;
        if (x->ui.mode == M_SAVE && x->ui.lsState == 1) { x->ui.lsYes = (uint8_t)(steps > 0); x->displayRev++; return; }
        if (x->ui.mode == M_SAVE && x->ui.lsState == 2) {
            int c = x->ui.nameCursor + steps;
            x->ui.nameCursor = (uint8_t)(c < 0 ? 0 : c > NAME_LEN ? NAME_LEN : c);
            x->displayRev++;
            return;
        }
        {
            int s = x->ui.lsSlot + steps;
            x->ui.lsSlot = (uint8_t)(s < 0 ? 0 : s > 63 ? 63 : s);
        }
        x->displayRev++;
        return;
    case M_SONG:
        if (x->ui.sel == 0) slot_set(x, &x->slots[0], slot_value(x, &x->slots[0]) + steps);
        else slot_set(x, &x->slots[1], slot_value(x, &x->slots[1]) + steps);
        return;
    case M_CLEAR:
        slot_set(x, &x->slots[0], slot_value(x, &x->slots[0]) + steps);
        return;
    default: {
        /* move the selection over the shown parameters (2.3 menu navigation) */
        int i = x->ui.sel, n = 0;
        while (n < NUM_SLOTS) {
            i += steps > 0 ? 1 : -1;
            if (i < 0 || i >= NUM_SLOTS) break;
            if (x->slots[i].kind) { x->ui.sel = (uint8_t)i; steps += steps > 0 ? -1 : 1; if (!steps) break; }
            n++;
        }
        x->ui.potHalf = (uint8_t)(x->ui.sel / 4);
        x->displayRev++;
        return;
    }
    }
}

/* the DATA encoder's position in what it moves through (0..1), so its knob turns from one end to the other: the
 * selection over the page's values, the slot list, a name's cursor, a song row */
static int data_range(Lxr *x, int *pos)
{
    int i, n = 0;
    switch (x->ui.mode) {
    case M_LOAD: case M_SAVE:
        if (x->ui.lsType == 4) return 0;
        if (x->ui.mode == M_SAVE && x->ui.lsState == 1) { *pos = x->ui.lsYes; return 2; }
        if (x->ui.mode == M_SAVE && x->ui.lsState == 2) { *pos = x->ui.nameCursor; return NAME_LEN + 1; }
        *pos = x->ui.lsSlot;
        return 64;
    case M_SONG: case M_CLEAR: {
        const Slot *s = &x->slots[x->ui.mode == M_SONG && x->ui.sel ? 1 : 0];
        if (!s->kind) return 0;
        *pos = slot_value(x, s);
        return slot_max(x, s) + 1;
    }
    default:
        *pos = 0;
        for (i = 0; i < NUM_SLOTS; i++)
            if (x->slots[i].kind) { if (i < x->ui.sel) (*pos)++; n++; }
        return n;
    }
}

static float data_pos(Lxr *x)
{
    int pos = 0, n = data_range(x, &pos);
    return n > 1 ? (float)pos / (float)(n - 1) : 0.5f;
}

/* The DATA knob turns with what it moves through, but only when DATA itself is turned: a pot selecting its value (which
 * moves the selection) leaves the knob where it is. dataKnob holds its place + 1 (0 = not read yet). */
float ui_dataGet(Lxr *x)
{
    if (x->ui.dataKnob <= 0.f) x->ui.dataKnob = data_pos(x) + 1.f;   /* first look: where the selection is */
    return x->ui.dataKnob - 1.f;
}

void ui_dataSet(Lxr *x, float v)
{
    /* a move from the position read back: a small one (a Q-Link event, a wheel click) is one step, a larger one (a drag)
     * as many steps as it covers */
    int pos = 0, n = data_range(x, &pos), steps;
    float d;
    if (n < 2) {   /* nothing to move through: the plain relative encoder */
        d = v - ui_dataGet(x);
        if (d > -1e-4f && d < 1e-4f) return;
        ui_data(x, d > 0 ? 1 : -1);
        return;
    }
    /* the move is measured from where the knob's picture stands (what MPC read back), and goes from the selection */
    d = (v - ui_dataGet(x)) * (float)(n - 1);
    if (d > -0.002f && d < 0.002f) {
        /* the knob's picture is at an end MPC can't go past, but the selection isn't: push on from there */
        const float at = data_pos(x);
        if (v >= 0.999f && at < 0.999f) d = 1.f;
        else if (v <= 0.001f && at > 0.001f) d = -1.f;
        else return;
    }
    steps = (int)(d + (d > 0 ? 0.5f : -0.5f));
    if (steps == 0) steps = d > 0 ? 1 : -1;
    ui_data(x, steps);
    {   /* the picture turns by the steps taken, like the encoder it is */
        float k = ui_dataGet(x) + (float)steps / (float)(n - 1);
        x->ui.dataKnob = (k < 0.f ? 0.f : k > 1.f ? 1.f : k) + 1.f;
    }
}

static int pot_slot(Lxr *x, int pot)
{
    /* saving with a name being edited, quick naming (7.4): pot 1 the cursor, 2 the case, 3 the letter */
    if (x->ui.mode == M_SAVE && x->ui.lsState == 2) return pot < 3 ? pot + 1 : -1;
    return x->ui.potHalf * 4 + pot;
}

float ui_potGet(Lxr *x, int pot)
{
    const int i = pot >= 0 && pot < 4 ? pot_slot(x, pot) : -1;
    return i >= 0 && x->slots[i].kind ? ui_slotGet(x, i) : 0.f;
}

void ui_potSet(Lxr *x, int pot, float v)
{
    /* a pot stands on the value it edits (0 = its minimum, full right = its maximum) and turns it like its slider */
    const int i = pot >= 0 && pot < 4 ? pot_slot(x, pot) : -1;
    if (i < 0 || !x->slots[i].kind) return;
    ui_slotSet(x, i, v);
}

void ui_pot(Lxr *x, int pot, int steps)
{
    const int i = x->ui.potHalf * 4 + pot;
    const Slot *s;
    if (pot < 0 || pot > 3 || !steps) return;
    if (x->ui.mode == M_SAVE && x->ui.lsState == 2) {
        /* quick naming (7.4): knob 1 cursor, 2 case, 3 letter */
        const Slot ns[3] = {SLOT(SK_NAMEPOS, 0), SLOT(SK_NAMECASE, 0), SLOT(SK_NAMECHR, 0)};
        if (pot < 3) slot_set(x, &ns[pot], slot_value(x, &ns[pot]) + steps);
        return;
    }
    s = &x->slots[i];
    if (!s->kind) return;
    x->ui.sel = (uint8_t)i;
    slot_set(x, s, slot_value(x, s) + steps);
}

/* ---------------------------------------------------------------- LEDs */
static int blink(Lxr *x) { return (lxr_clock(x) / 5512u) & 1u; }   /* 4 Hz */

int ui_led(Lxr *x, int b)
{
    const uint8_t m = x->ui.mode;
    const uint32_t clk = lxr_clock(x);
    if (b == B_SHIFT) return shifted(x);
    /* the encoder's push (the "DATA" legend) blinks while it has something to confirm */
    if (b == B_ENC) return ((m == M_LOAD || m == M_SAVE) && x->ui.lsType != 4) || m == M_CLEAR || m == M_PADLEARN ? blink(x) : 0;
    if (b == B_PLAY) return x->seq.running ? 1 : (x->seq.songMode && blink(x));   /* lit steadily while it runs */
    if (b == B_REC) return x->seq.recording ? (x->seq.erasing ? blink(x) : 1) : 0;
    if (b == B_COPY) return x->ui.copy ? blink(x) : (m == M_CLEAR ? blink(x) : 0);
    if (b == B_VOICE) return m == M_VOICE ? 1 : m == M_STEP ? blink(x) : 0;
    if (b == B_PERF) return m == M_PERF ? 1 : m == M_PATGEN ? blink(x) : 0;
    if (b == B_LOAD) return m == M_LOAD || x->ui.lsArm == 1 ? (x->ui.lsArm == 1 ? blink(x) : 1) : 0;
    if (b == B_SAVE) return m == M_SAVE || x->ui.lsArm == 2 ? (x->ui.lsArm == 2 ? blink(x) : 1) : 0;
    if (b >= B_SEL1 && b <= B_SEL8) return m == M_VOICE && x->ui.section == b - B_SEL1;
    if (b == B_BARL || b == B_BARR) {
        /* the bar shown, as two bits (2.1): 1st none, 2nd right, 3rd left, 4th both; pattern pages: the group */
        const int pages = m == M_PATTERN || m == M_SONG || ((m == M_LOAD || m == M_SAVE) && x->ui.lsType == 4);
        const uint8_t v = pages ? x->ui.group : x->ui.bar;
        return b == B_BARR ? (v & 1) != 0 : (v & 2) != 0;
    }
    if (b >= B_V1 && b <= B_V7) {
        const uint8_t t = (uint8_t)(b - B_V1);
        int lit;
        if (x->ui.copy && x->ui.copySrc == t) return blink(x);
        if (m == M_PERF || shifted(x)) lit = !(x->seq.mutes & (1u << t));   /* unmuted voices light */
        else if (m == M_LASTSTEP && x->ui.lastStepAll) lit = 1;
        else lit = x->ui.track == t;
        if (x->ui.pulse[t] && (int32_t)(x->ui.pulse[t] - clk) > 0) lit = !lit;   /* the voice pulses on a hit */
        return lit;
    }
    if (b >= B_S1 && b <= B_S16) {
        const uint8_t k = (uint8_t)(b - B_S1);
        const uint8_t t = x->ui.track;
        if (x->ui.lsArm) return k == 7 || k == 8 || k == 9 || k == 10 || k == 15;
        if (shifted(x) && m == M_VOICE && k >= 9 && k != 10) return 0;
        switch (m) {
        case M_VOICE: case M_PATGEN: case M_STEP: {
            const uint8_t s = (uint8_t)(x->ui.bar * 16 + k);
            const uint8_t len = x->pat.len[t] ? x->pat.len[t] : 16;
            int lit = s < len && x->pat.s[t][s].on;
            if (m == M_STEP && s == x->ui.step) return blink(x);
            if (x->ui.heldStep == (int8_t)s) return blink(x);
            if (x->seq.running && x->seq.lastPos[t] == s) lit = !lit;   /* the chaselight */
            return lit;
        }
        case M_PERF:
            if (k < NUM_TRACKS) return (x->seq.rolls >> k) & 1u;
            return 0;
        case M_PATTERN: case M_LOAD: case M_SAVE: {
            const uint8_t pat = (uint8_t)(x->ui.group * 16 + k);
            if (m != M_PATTERN && x->ui.lsType != 4) return k == ls_key(x->ui.lsType) ? blink(x) : 0;
            if (x->seq.nextPat == pat) return blink(x);
            return x->patSlot == pat;
        }
        case M_SONG: return x->p.songs[x->songSlot].e[x->ui.songRow] == x->ui.group * 16 + k;
        case M_LASTSTEP: {
            const uint8_t len = x->pat.len[t] ? x->pat.len[t] : 16;
            return (uint8_t)(x->ui.bar * 16 + k + 1) == len;
        }
        case M_CONFIG: return k == 11 ? blink(x) : 0;
        case M_FX: return k == 14 ? blink(x) : 0;
        default: return 0;
        }
    }
    return 0;
}

/* ---------------------------------------------------------------- the display */
static void name8(const char *n, char *out) { int i; for (i = 0; i < NAME_LEN; i++) out[i] = n[i] >= 32 && n[i] < 127 ? n[i] : ' '; out[NAME_LEN] = 0; while (out[0] && out[strlen(out) - 1] == ' ') out[strlen(out) - 1] = 0; }

void ui_lcd(Lxr *x, char *l1, char *l2)
{
    const uint8_t m = x->ui.mode;
    char v[24], nm[NAME_LEN + 1];
    const Slot *s = &x->slots[x->ui.sel];
    if (x->ui.msgUntil && (int32_t)(x->ui.msgUntil - lxr_clock(x)) > 0) {
        strcpy(l1, x->ui.msg[0]);
        strcpy(l2, x->ui.msg[1]);
        return;
    }
    if (x->ui.lsArm) {
        sprintf(l1, "%s:", x->ui.lsArm == 1 ? "Load" : "Save");
        strcpy(l2, "Kit Mrp Sng Prj Pat");
        return;
    }
    switch (m) {
    case M_LOAD: case M_SAVE: {
        const char *what = lsNames[x->ui.lsType];
        if (x->ui.lsType == 4) {
            sprintf(l1, "%s Pattern", m == M_LOAD ? "Select" : "Save");
            sprintf(l2, "Current: %d", x->patSlot + 1);
            return;
        }
        sprintf(l1, "%s %s", m == M_LOAD ? "Load:" : "Save:", what);
        if (m == M_SAVE && x->ui.lsState == 1) { strcpy(l2, x->ui.lsYes ? "Edit Name? [Y/n]" : "Edit Name? [y/N]"); return; }
        if (m == M_SAVE && x->ui.lsState == 2) {
            int i;
            char buf[24];
            name8(x->ui.name, nm);
            memcpy(buf, x->ui.name, NAME_LEN);
            for (i = 0; i < NAME_LEN; i++) if (buf[i] < 32 || buf[i] > 126) buf[i] = ' ';
            buf[NAME_LEN] = 0;
            if (x->ui.nameCursor < NAME_LEN) {
                char t[24];
                snprintf(t, sizeof t, "%.*s[%c]%s  ok", x->ui.nameCursor, buf, buf[x->ui.nameCursor], buf + x->ui.nameCursor + 1);
                strcpy(l2, t);
            } else
                sprintf(l2, "%s >ok", buf);
            return;
        }
        switch (x->ui.lsType) {
        case 0: case 1: name8(x->p.kits[x->ui.lsSlot].name, nm); break;
        case 2: name8(x->p.songs[x->ui.lsSlot].name, nm); break;
        default: if (!store_projectName(x, x->ui.lsSlot, nm)) strcpy(nm, "Empty"); break;
        }
        sprintf(l2, "[%d] %s", x->ui.lsSlot + 1, nm[0] ? nm : "Empty");
        return;
    }
    case M_PATTERN:
        strcpy(l1, "Select Patterns");
        if (x->seq.nextPat != 0xff) sprintf(l2, "Current: %d > %d", x->patSlot + 1, x->seq.nextPat + 1);
        else sprintf(l2, "Current: %d", x->patSlot + 1);
        return;
    case M_SONG: {
        const Song *sg = &x->p.songs[x->songSlot];
        char a[8], c[8];
        const uint8_t r = x->ui.songRow;
        slot_text(x, &x->slots[1], a);
        if (r + 1 < SONG_LEN) {
            const Slot nx = SLOT(SK_SONGPAT, 0);
            uint8_t save = x->ui.songRow;
            x->ui.songRow = (uint8_t)(r + 1);
            slot_text(x, &nx, c);
            x->ui.songRow = save;
        } else c[0] = 0;
        (void)sg;
        sprintf(l1, "%c[%03d] %s", x->ui.sel == 0 ? '>' : ' ', r, a);
        sprintf(l2, " %03d  %s", r + 1, c);
        if (x->ui.sel == 1) sprintf(l1, " %03d [%s]", r, a);
        return;
    }
    case M_CLEAR:
        strcpy(l1, "clear");
        sprintf(l2, "[%s]?", clearNames[x->ui.clearSel]);
        return;
    case M_LASTSTEP:
        strcpy(l1, "Last Step");
        if (x->ui.lastStepAll) sprintf(l2, "Trk: All  %d", (x->pat.len[0] ? x->pat.len[0] : 16));
        else sprintf(l2, "Trk: %d  %d", x->ui.track + 1, x->pat.len[x->ui.track] ? x->pat.len[x->ui.track] : 16);
        return;
    case M_PADLEARN:
        strcpy(l1, "Pad learn");
        if (x->ui.learnStage == 0) strcpy(l2, "hit bottom-left pad");
        else if (x->ui.learnStage == 1) sprintf(l2, "%d > pad right of it", x->ui.learnNotes[0]);
        else sprintf(l2, "%d %d > pad above", x->ui.learnNotes[0], x->ui.learnNotes[1]);
        return;
    default: break;
    }
    /* parameter pages: what is edited, then the selected parameter's long name and value (the detail page, 2.3) */
    switch (m) {
    case M_VOICE: sprintf(l1, "%s %s", trackNames[x->ui.track], sectionNames[x->ui.section]); break;
    case M_STEP: sprintf(l1, "%s Step %d", trackNames[x->ui.track], x->ui.step + 1); break;
    case M_PERF: strcpy(l1, "Performance"); break;
    case M_PATGEN: sprintf(l1, "Euklid %s", trackNames[x->ui.track]); break;
    case M_CONFIG: strcpy(l1, "Global"); break;
    case M_FX: strcpy(l1, "FX"); break;
    case M_REC: strcpy(l1, "Record"); break;
    default: l1[0] = 0; break;
    }
    {
        char right[16];
        sprintf(right, " P%02d", x->patSlot + 1);
        if (strlen(l1) + strlen(right) < 22) strcat(l1, right);
    }
    if (s->kind) {
        slot_text(x, s, v);
        sprintf(l2, "%s %s", slot_long(x, s), v);
    } else
        l2[0] = 0;
}

/* ---------------------------------------------------------------- pads */
/* The user's Force layout (bottom row up): row 1 = steps 1-8, row 2 = steps 9-16, row 3 = SHIFT PLAY REC COPY VOICE
 * PERF LOAD SAVE, row 4 = OSC .. MIX, row 5 = BAR< BAR> DRUM1 DRUM2 DRUM3 SNARE CLP/CYM CL HH, row 6 pad 8 = OP HH. */
static const int8_t padMap[6][8] = {
    {B_S1, B_S2, B_S3, B_S4, B_S5, B_S6, B_S7, B_S8},
    {B_S9, B_S10, B_S11, B_S12, B_S13, B_S14, B_S15, B_S16},
    {B_SHIFT, B_PLAY, B_REC, B_COPY, B_VOICE, B_PERF, B_LOAD, B_SAVE},
    {B_SEL1, B_SEL2, B_SEL3, B_SEL4, B_SEL5, B_SEL6, B_SEL7, B_SEL8},
    {B_BARL, B_BARR, B_V1, B_V2, B_V3, B_V4, B_V5, B_V6},
    {-1, -1, -1, -1, -1, -1, -1, B_V7},
};

/* the pad a note comes from in the learned layout: note = base + col * colStep + row * rowStep */
static int pad_at(const Lxr *x, int note, int *row, int *col)
{
    int r, c;
    for (r = 0; r < 6; r++)
        for (c = 0; c < 8; c++)
            if (x->padBase + c * x->padCol + r * x->padRow == note) { *row = r; *col = c; return 1; }
    return 0;
}

int ui_padButton(Lxr *x, uint8_t note)
{
    int row, col;
    if (!x->padLearned || x->padMode != 0) return -1;
    if (!pad_at(x, note, &row, &col)) return -1;
    return padMap[row][col];
}

/* how many of the 48 pads share a note with another one (a layout whose rows overlap, e.g. rows a 4th apart) */
static int pad_clashes(const Lxr *x)
{
    int seen[128] = {0}, r, c, n = 0;
    for (r = 0; r < 6; r++)
        for (c = 0; c < 8; c++) {
            const int note = x->padBase + c * x->padCol + r * x->padRow;
            if (note < 0 || note > 127) { n++; continue; }
            if (seen[note]++) n++;
        }
    return n;
}

void ui_padNote(Lxr *x, uint8_t note, uint8_t vel)
{
    int b;
    if (x->ui.mode == M_PADLEARN) {
        if (!vel) return;
        /* three pads: the bottom-left one, the one right of it, the one above it */
        if (x->ui.learnStage < 2) {
            if (x->ui.learnStage == 1 && note == x->ui.learnNotes[0]) return;
            x->ui.learnNotes[x->ui.learnStage++] = note;
            x->displayRev++;
            return;
        }
        if (note == x->ui.learnNotes[0] || note == x->ui.learnNotes[1]) return;
        x->ui.learnNotes[2] = note;
        x->padBase = x->ui.learnNotes[0];
        x->padCol = (int8_t)(x->ui.learnNotes[1] - x->ui.learnNotes[0]);
        x->padRow = (int8_t)(note - x->ui.learnNotes[0]);
        x->padLearned = 1;
        x->padMode = 0;
        {
            char l2[32];
            const int clash = pad_clashes(x);
            snprintf(l2, sizeof l2, "%d %d %d%s", x->ui.learnNotes[0], x->ui.learnNotes[1], note, clash ? " overlap!" : " ok");
            ui_message(x, clash ? "Pads overlap: set" : "Pads learned", clash ? "chromatic, no overlap" : l2, 3000);
            if (clash) x->padLearned = 0;   /* an overlapping layout can't be the panel: the pads keep playing the voices */
        }
        store_saveSettings(x);
        x->ui.mode = x->ui.prevMode == M_PADLEARN ? M_CONFIG : x->ui.prevMode;
        ui_refreshSlots(x);
        return;
    }
    b = ui_padButton(x, note);
    if (b < 0) return;
    if (vel) ui_press(x, b, 1);
    else ui_release(x, b, 1);
}

void ui_init(Lxr *x)
{
    int t;
    memset(&x->ui, 0, sizeof x->ui);
    x->ui.mode = M_VOICE;
    x->ui.prevMode = M_VOICE;
    x->ui.heldStep = -1;
    x->ui.copySrc = 0xff;
    for (t = 0; t < NUM_TRACKS; t++) { x->ui.genLen[t] = 16; x->ui.genSteps[t] = 4; x->ui.genRot[t] = 0; }
    ui_refreshSlots(x);
}

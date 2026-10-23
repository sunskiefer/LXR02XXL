/* LXRXXL -- the plugin engine (mpc-vst-plugins wrapper/engine.h): parameters, MIDI and audio for one LXR-02.
 *
 * Every call runs with this instance's copy of the LXR engine state in place (lxr_state.c). The skin's buttons are text
 * parameters whose "<key>_on" is the button's LED; the wrapper polls those every 10 ms and repaints the keys. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "engine.h"
#include "lxr.h"
#include "lxr_state.h"
#include "param_ids.h"
#include "plugin_dir.h"

typedef struct {
    Lxr x;
    LxrState *st;
} Inst;

/* ---- the data folder: MODULE_DIR (outside the plugin folder, so projects survive updates), else next to the .so ---- */
static int writable_dir(const char *d)
{
    char probe[700];
    FILE *f;
    char tmp[600];
    char *p;
    snprintf(tmp, sizeof tmp, "%s", d);
    for (p = tmp + 1; *p; p++) if (*p == '/') { *p = 0; mkdir(tmp, 0755); *p = '/'; }
    mkdir(tmp, 0755);
    snprintf(probe, sizeof probe, "%s/.probe", d);
    f = fopen(probe, "w");
    if (!f) return 0;
    fclose(f);
    remove(probe);
    return 1;
}

static void *create(const char *dataDir)
{
    Inst *in = calloc(1, sizeof *in);
    char dir[600] = "";
    if (!in) return NULL;
    in->st = lxr_state_create();
    if (!in->st) { free(in); return NULL; }
    if (dataDir && writable_dir(dataDir)) snprintf(dir, sizeof dir, "%s", dataDir);
    else {
        char here[512];
        if (mpc_plugin_dir(here, sizeof here)) {
            snprintf(dir, sizeof dir, "%s/LXRXXL Data", here);
            if (!writable_dir(dir)) dir[0] = 0;
        }
    }
    lxr_state_enter(in->st);
    lxr_init(&in->x, dir);
    in->x.nrpn = -1;
    lxr_state_leave();
    return in;
}

static void destroy(void *p)
{
    Inst *in = p;
    store_forget(&in->x);
    lxr_state_destroy(in->st);
    free(in);
}

/* ---- MIDI ---- */
static void midi_cc(Lxr *x, uint8_t cc, uint8_t v)
{
    /* the LXR's CC list (9.7): CC n sets sound parameter n - 1; NRPN (99/98 + 6) the second page (9.8) */
    int nrpn = x->nrpn;
    if (cc == 99) { x->nrpn = (v << 7) | (nrpn >= 0 ? nrpn & 127 : 0); return; }
    if (cc == 98) { x->nrpn = (nrpn >= 0 ? nrpn & ~127 : 0) | v; return; }
    if (cc == 6 && nrpn >= 0) {
        /* NRPN n is parameter 128 + n: up to the sound parameters' end, then pulse widths, the FX unit (100..117) */
        if (nrpn <= LXR_NUM_SOUND_PARAMS - 128 - 1) lxr_setKitParam(x, (uint16_t)(128 + nrpn), v, 1);
        else if (nrpn < LXR_KIT_BYTES - 128 - 1) lxr_setExt(x, (uint8_t)(128 + nrpn - LXR_NUM_SOUND_PARAMS), v);
        x->displayRev++;
        return;
    }
    if (cc == 1) {   /* mod wheel: morph (the LXR02Open "CC1 -> MORPH") */
        x->morph = (uint8_t)(v << 1);
        lxr_applyKit(x);
        return;
    }
    if (cc >= 2 && cc <= 127 && cc != 5 && cc != 98 && cc != 99) {
        lxr_setKitParam(x, (uint16_t)(cc - 1), v, 1);
        x->displayRev++;
    }
}

static void midi(void *p, const uint8_t *m, int len)
{
    Inst *in = p;
    Lxr *x = &in->x;
    const uint8_t st = m[0] & 0xF0, ch = m[0] & 0x0F;
    uint8_t t;
    if (len < 2) return;
    lxr_state_enter(in->st);
    if (st == 0x90 || st == 0x80) {
        const uint8_t note = m[1] & 127, vel = st == 0x90 ? (m[2] & 127) : 0;
        int handled = 0;
        if (x->ui.mode == M_PADLEARN) { ui_padNote(x, note, vel); handled = 1; }
        else if (ui_padButton(x, note) >= 0) { ui_padNote(x, note, vel); handled = 1; }
        if (!handled && vel && (x->p.g.mrx & 1)) {
            if (x->ui.mode == M_STEP) {   /* a keyboard enters the selected step's note (7.2) */
                x->pat.s[x->ui.track][x->ui.step].note = note;
                x->displayRev++;
            }
            /* The drums, in pad order: pad 1 = DRUM1 ... pad 7 = OP HH. Each voice answers its own note (MIX page; by
             * default the Force's pads 1-7 in its default layout, C major from C1: lxr_padNotes) and, for the drum
             * layout of the mpc-vst-plugins drum-pad patch (pad n sends note n - 1), notes 0-6. So the Force's step
             * sequencer and its pads play the LXR's voices in the order of its voice keys. */
            int took = 0;
            for (t = 0; t < NUM_TRACKS; t++) {
                const uint8_t want = x->p.g.trackNote[t];
                if (x->p.g.trackCh[t] != ch) continue;
                if (want == 0) { lxr_trigger(x, t, vel, note, 1); took = 1; }          /* "any": played tonally */
                else if (want == note || note == t) { lxr_trigger(x, t, vel, 63, 1); took = 1; }
            }
            /* the global channel plays the selected voice tonally with any other note (not the unused pads 8-16 of
             * either layout: notes 7-15, or 36-63 for the C major pads) */
            if (!took && ch == x->p.g.midiCh && note > 15 && (note < 36 || note > 63))
                lxr_trigger(x, x->ui.track, vel, note, 1);
            if (!x->seq.hostPlaying && !x->seq.running) {   /* a pad hit by hand: show what it played */
                char l2[32];
                for (t = 0; t < NUM_TRACKS; t++)
                    if (x->p.g.trackCh[t] == ch && (x->p.g.trackNote[t] == note || note == t)) break;
                snprintf(l2, sizeof l2, "note %d > %s", note, t < NUM_TRACKS ? ui_trackName(t) : took ? "voice" : "-");
                ui_message(x, "MIDI in", l2, 700);
            }
        }
    } else if (st == 0xB0 && len >= 3 && (x->p.g.mrx & 4) && ch == x->p.g.midiCh) {
        midi_cc(x, m[1] & 127, m[2] & 127);
    } else if (st == 0xC0 && (x->p.g.mrx & 8) && ch == x->p.g.midiCh) {
        seq_queuePattern(x, (uint8_t)(m[1] & 63));   /* program change: the pattern */
    }
    lxr_state_leave();
}

/* ---- parameters ---- */
static int key_index(const char *k, const char **suffix)
{
    /* the plugin's keys and the wrapper's "<key>_on" / "_name" / "_display" */
    int i;
    for (i = 0; i < P_COUNT; i++) {
        const size_t n = strlen(PARAM_KEYS[i]);
        if (!strncmp(k, PARAM_KEYS[i], n) && (k[n] == 0 || k[n] == '_')) {
            if (k[n] == '_') {
                /* "slot1_used" is its own parameter, not "slot1" + "_used" */
                int j;
                for (j = 0; j < P_COUNT; j++) if (!strcmp(k, PARAM_KEYS[j])) { *suffix = ""; return j; }
            }
            *suffix = k + n;
            return i;
        }
    }
    *suffix = "";
    return -1;
}


static void set_param(void *p, const char *k, const char *v)
{
    Inst *in = p;
    Lxr *x = &in->x;
    const char *sfx;
    int i;
    lxr_state_enter(in->st);
    if (!strcmp(k, "state")) { store_deserialize(x, v); lxr_state_leave(); return; }
    if (!strcmp(k, "host_time")) {
        int playing = 0;
        double ppq = 0, tempo = 0;
        if (sscanf(v, "%d %lf %lf", &playing, &ppq, &tempo) == 3) seq_hostTime(x, playing, ppq, tempo);
        lxr_state_leave();
        return;
    }
    i = key_index(k, &sfx);
    if (i < 0 || *sfx) { lxr_state_leave(); return; }
    if (i >= P_FADER1 && i <= P_FADER7) x->fader[i - P_FADER1] = (float)atof(v);
    else if (i == P_DATA) ui_dataSet(x, (float)atof(v));
    else if (i >= P_POT1 && i <= P_POT4) ui_potSet(x, i - P_POT1, (float)atof(v));
    else if (i >= P_SLOT1 && i <= P_SLOT8) ui_slotSet(x, i - P_SLOT1, (float)atof(v));
    else if (i == P_RND || i == P_SRND) {
        /* the mod's keys: a tap randomizes everything, with SHIFT only the selected voice; the key lights a moment */
        const int one = x->ui.shift || x->ui.shiftHeld;
        if (i == P_RND) {
            lxr_randomizeVoices(x, x->rnd.amt * 10, one ? lxr_dsp_voiceOfTrack(x->ui.track) : -1);
            x->rnd.flash = lxr_clock(x) + 13230;
        } else {
            seq_randomize(x, x->rnd.seqAmt * 10, one ? x->ui.track : -1);
            x->rnd.seqFlash = lxr_clock(x) + 13230;
        }
        x->ui.shift = 0;   /* a latched SHIFT is spent */
        ui_refreshSlots(x);
    }
    else if (i == P_RESET) {   /* RESET: the factory project, as a new LXRXXL starts */
        lxr_loadFactory(x);
        ui_init(x);
        x->rnd.resetFlash = lxr_clock(x) + 13230;
        ui_message(x, "Reset", "factory project", 1200);
    }
    else if (i == P_RND_AMT || i == P_SRND_AMT) {
        int a = atoi(v);
        a = a < 0 ? 0 : a > 10 ? 10 : a;
        if (i == P_RND_AMT) x->rnd.amt = (uint8_t)a; else x->rnd.seqAmt = (uint8_t)a;
        x->displayRev++;
    }
    else if (i >= P_FIRST_BUTTON && i < P_FIRST_BUTTON + P_NUM_BUTTONS) {
        /* a tap on a key: whatever value the host sends, it is a press (the key's picture is its LED, which the
         * host toggled on its own: report the real LED once more after the tap so the picture follows it) */
        const int b = i - P_FIRST_BUTTON;
        ui_press(x, b, 0);
        x->pressFlip[b] = lxr_clock(x) + 600;
    }
    lxr_state_leave();
}

static int put(char *buf, int n, const char *s)
{
    const int len = (int)strlen(s);
    if (n <= 0) return 0;
    snprintf(buf, (size_t)n, "%s", s);
    return len > 0 ? len : 0;
}

static int get_param(void *p, const char *k, char *buf, int n)
{
    Inst *in = p;
    Lxr *x = &in->x;
    const char *sfx;
    char t[64];
    int i, r = 0;
    lxr_state_enter(in->st);
    if (!strcmp(k, "state")) {
        int len = 0;
        char *s = store_serialize(x, &len);
        if (s) {
            if (len < n) memcpy(buf, s, (size_t)len + 1);
            r = len + 1;   /* the wrapper grows its buffer when this is larger than what it gave */
            free(s);
        }
        lxr_state_leave();
        return r;
    }
    if (!strcmp(k, "display_rev")) {
        snprintf(t, sizeof t, "%u", (unsigned)x->displayRev);
        r = put(buf, n, t);
        lxr_state_leave();
        return r;
    }
    i = key_index(k, &sfx);
    if (i < 0) { lxr_state_leave(); return 0; }
    if (i >= P_FADER1 && i <= P_FADER7 && !*sfx) { snprintf(t, sizeof t, "%.4f", x->fader[i - P_FADER1]); r = put(buf, n, t); }
    else if (i == P_DATA && !*sfx) { snprintf(t, sizeof t, "%.5f", ui_dataGet(x)); r = put(buf, n, t); }
    else if (i >= P_POT1 && i <= P_POT4 && !*sfx) { snprintf(t, sizeof t, "%.5f", ui_potGet(x, i - P_POT1)); r = put(buf, n, t); }
    else if (i >= P_POT1 && i <= P_POT4 && !strcmp(sfx, "_display")) {
        /* the value this pot edits: the page's value 1-4 (or 5-8) */
        const int s = x->ui.potHalf * 4 + (i - P_POT1);
        if (ui_slotUsed(x, s)) ui_slotDisplay(x, s, t); else strcpy(t, " ");
        r = put(buf, n, t[0] ? t : " ");
    }
    else if (i >= P_SLOT1 && i <= P_SLOT8) {
        const int s = i - P_SLOT1;
        if (!strcmp(sfx, "_name")) { ui_slotName(x, s, t); r = put(buf, n, t[0] ? t : " "); }
        else if (!strcmp(sfx, "_display")) { ui_slotDisplay(x, s, t); r = put(buf, n, t[0] ? t : " "); }
        else if (!*sfx) { snprintf(t, sizeof t, "%.5f", ui_slotGet(x, s)); r = put(buf, n, t); }
    } else if (i >= P_SLOT1_USED && i <= P_SLOT8_USED && !*sfx) r = put(buf, n, ui_slotUsed(x, i - P_SLOT1_USED) ? "1" : "0");
    else if (i == P_POTS && !*sfx) r = put(buf, n, x->ui.potHalf ? "1" : "0");
    else if ((i == P_LCD1 || i == P_LCD2) && !*sfx) {
        char l1[48], l2[48];
        ui_lcd(x, l1, l2);
        r = put(buf, n, i == P_LCD1 ? (l1[0] ? l1 : " ") : (l2[0] ? l2 : " "));
    } else if (i == P_RND || i == P_SRND || i == P_RESET) {
        const uint32_t until = i == P_RND ? x->rnd.flash : i == P_SRND ? x->rnd.seqFlash : x->rnd.resetFlash;
        if (!strcmp(sfx, "_on")) r = put(buf, n, until && (int32_t)(until - lxr_clock(x)) > 0 ? "1" : "0");
        else if (!*sfx) r = put(buf, n, " ");
    } else if ((i == P_RND_AMT || i == P_SRND_AMT) && !*sfx) {
        snprintf(t, sizeof t, "%d", i == P_RND_AMT ? x->rnd.amt : x->rnd.seqAmt);
        r = put(buf, n, t);
    } else if (i >= P_FIRST_BUTTON && i < P_FIRST_BUTTON + P_NUM_BUTTONS) {
        const int b = i - P_FIRST_BUTTON;
        if (!strcmp(sfx, "_on")) {
            int on = ui_led(x, b);
            if (x->pressFlip[b] && (int32_t)(x->pressFlip[b] - lxr_clock(x)) > 0) on = !on;
            else x->pressFlip[b] = 0;
            r = put(buf, n, on ? "1" : "0");
        } else if (!*sfx) r = put(buf, n, " ");
    }
    lxr_state_leave();
    return r;
}

static void render(void *p, int16_t *out, int frames)
{
    Inst *in = p;
    float L[128], R[128];
    int i;
    if (frames > 128) frames = 128;
    lxr_state_enter(in->st);
    store_poll(&in->x);
    lxr_render(&in->x, L, R, frames);
    lxr_state_leave();
    for (i = 0; i < frames; i++) {
        float l = L[i] * 32767.f, r = R[i] * 32767.f;
        out[2 * i] = (int16_t)(l > 32767.f ? 32767 : l < -32768.f ? -32768 : (int)l);
        out[2 * i + 1] = (int16_t)(r > 32767.f ? 32767 : r < -32768.f ? -32768 : (int)r);
    }
}

static const mpc_engine_t ENGINE = {create, destroy, midi, set_param, get_param, render, NULL};
const mpc_engine_t *mpc_engine(void) { return &ENGINE; }

/* LXRXXL -- one instance: the init kit and pattern, kit edits into the engine, triggers and the render loop. */
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "lxr.h"
#include "Parameters.h"
#include "menu/menu.h"

/* ---------------------------------------------------------------- the init kit
 * "Clear kit" reverts to the init sound (7.1): the LXR-02's own init kit (initKit.c, from the firmware: its name, then
 * the kit's parameters). */
#include "Preset/InitKit.h"

void lxr_initKit(Kit *k)
{
    int i;
    memset(k, 0, sizeof *k);
    memcpy(k->name, "Init    ", NAME_LEN);
    for (i = 8; i < INIT_KIT_SIZE && i - 8 < LXR_KIT_BYTES; i++) k->b[i - 8] = initKit[i];
}

static void voice_common(uint8_t *b, int v)
{
    static const uint8_t pans[6] = {PAR_PAN1, PAR_PAN2, PAR_PAN3, PAR_PAN4, PAR_PAN5, PAR_PAN6};
    b[PAR_FINE1 + 2 * v] = 63;
    b[PAR_VOL1 + v] = 100;
    b[pans[v]] = 63;
    b[PAR_VOICE_DECIMATION1 + v] = 127;
    b[PAR_AUDIO_OUT1 + v] = 0;
    b[PAR_FILTER_FREQ_1 + v] = 127;
    b[PAR_FILTER_TYPE_1 + v] = 0;
    b[PAR_VOLUME_MOD_ON_OFF1 + v] = 1;
    b[PAR_VOICE_LFO1 + v] = (uint8_t)(v + 1);
    b[PAR_TRANS1_FREQ + v] = 60;
    b[PAR_VOL_SLOPE1 + v] = 25;
}

/* LXRXXL 0.5.x's init kit, kept as the "Init 05" kit: built from the owner's manual's drum-synthesis recipes
 * (ch. 11): a kick, a tom, the 808 cowbell, the snare, the clap and closed/open hats. */
static void initKit05(Kit *k)
{
    uint8_t *b = k->b;
    int v;
    memset(k, 0, sizeof *k);
    memcpy(k->name, "Init 05 ", NAME_LEN);
    for (v = 0; v < 6; v++) voice_common(b, v);
    b[PAR_VOICE_DECIMATION_ALL] = 127;
    /* drum 1: kick (11.1): sine around 30-35, no attack, decay 20-30, slope ~25, pitch envelope alike */
    b[PAR_OSC_WAVE_DRUM1] = 0; b[PAR_COARSE1] = 32;
    b[PAR_VELOA1] = 0; b[PAR_VELOD1] = 28; b[PAR_VOL_SLOPE1] = 25;
    b[PAR_MOD_EG1] = 26; b[PAR_PITCH_SLOPE1] = 25; b[PAR_MODAMNT1] = 60;
    b[PAR_TRANS1_WAVE] = 0; b[PAR_TRANS1_VOL] = 30;
    b[PAR_FM_FREQ1] = 63; b[PAR_MOD_WAVE_DRUM1] = 0;
    /* drum 2: tom */
    b[PAR_OSC_WAVE_DRUM2] = 0; b[PAR_COARSE2] = 45;
    b[PAR_VELOA2] = 0; b[PAR_VELOD2] = 38; b[PAR_VOL_SLOPE2] = 30;
    b[PAR_MOD_EG2] = 34; b[PAR_PITCH_SLOPE2] = 30; b[PAR_MODAMNT2] = 30;
    b[PAR_TRANS2_WAVE] = 0; b[PAR_TRANS2_VOL] = 20;
    b[PAR_FM_FREQ2] = 63;
    /* drum 3: the 808 cowbell, to the manual's numbers (11.6) */
    b[PAR_MIX_MOD_3] = 1; b[PAR_FMAMNT3] = 63; b[PAR_MOD_WAVE_DRUM3] = 3; b[PAR_FM_FREQ3] = 78;
    b[PAR_OSC_WAVE_DRUM3] = 3; b[PAR_COARSE3] = 71;
    b[PAR_VELOA3] = 0; b[PAR_VOL_SLOPE3] = 3; b[PAR_VELOD3] = 35;
    b[PAR_FILTER_TYPE_3] = 2; b[PAR_RESO_3] = 70; b[PAR_FILTER_FREQ_3] = 92;
    /* snare (11.2): noise louder than the body, high-passed noise, a short exponential tail */
    b[PAR_OSC_WAVE_SNARE] = 0; b[PAR_COARSE4] = 52; b[PAR_NOISE_FREQ1] = 127; b[PAR_MIX1] = 100;
    b[PAR_VELOA4] = 0; b[PAR_VELOD4] = 45; b[PAR_VOL_SLOPE4] = 8;
    b[PAR_MOD_EG4] = 20; b[PAR_PITCH_SLOPE4] = 20; b[PAR_MODAMNT4] = 15;
    b[PAR_FILTER_TYPE_4] = 1; b[PAR_FILTER_FREQ_4] = 80; b[PAR_RESO_4] = 40;
    b[PAR_TRANS4_WAVE] = 0; b[PAR_TRANS4_VOL] = 10;
    /* clap / cymbal voice (11.3): noise at full frequency, 3-4 repeats, attack below 10, slope below 10, BP 60-80 */
    b[PAR_WAVE1_CYM] = 4; b[PAR_COARSE5] = 127;
    b[PAR_VELOA5] = 6; b[PAR_VELOD5] = 32; b[PAR_REPEAT5] = 3; b[PAR_VOL_SLOPE5] = 6;
    b[PAR_FILTER_TYPE_5] = 2; b[PAR_FILTER_FREQ_5] = 70; b[PAR_RESO_5] = 100;
    b[PAR_WAVE2_CYM] = 0; b[PAR_WAVE3_CYM] = 0;
    /* hi-hats (11.4): metallic FM, short decays (open longer than closed), slope below 10, high-pass around 120 */
    b[PAR_WAVE1_HH] = 3; b[PAR_COARSE6] = 100;
    b[PAR_MOD_OSC_F1] = 110; b[PAR_MOD_OSC_F2] = 92; b[PAR_MOD_OSC_GAIN1] = 100; b[PAR_MOD_OSC_GAIN2] = 80;
    b[PAR_WAVE2_HH] = 3; b[PAR_WAVE3_HH] = 3;
    b[PAR_VELOA6] = 0; b[PAR_VELOD6_CLOSED] = 12; b[PAR_VELOD6_OPEN] = 45; b[PAR_VOL_SLOPE6] = 5;
    b[PAR_FILTER_TYPE_6] = 1; b[PAR_FILTER_FREQ_6] = 120; b[PAR_RESO_6] = 10;
    /* the LXR-02's own: pulse widths at 50%, the FX unit off with a delay set up */
    for (v = 0; v < 6; v++) b[LXR_NUM_SOUND_PARAMS + EXT_PWM1 + v] = 64;
    b[LXR_NUM_SOUND_PARAMS + EXT_FX_TYPE] = 0;
    b[LXR_NUM_SOUND_PARAMS + EXT_FX_OUT] = 0;
    b[LXR_NUM_SOUND_PARAMS + EXT_FX_DW] = 64;
    b[LXR_NUM_SOUND_PARAMS + EXT_FX_P1] = 90;
    b[LXR_NUM_SOUND_PARAMS + EXT_FX_P2] = 1;
    b[LXR_NUM_SOUND_PARAMS + EXT_FX_P3] = 90;
    b[LXR_NUM_SOUND_PARAMS + EXT_FX_P4] = 60;
    b[LXR_NUM_SOUND_PARAMS + EXT_FX_RATIO] = 2;
}

void lxr_initPattern(Pattern *p)
{
    int t, s;
    memset(p, 0, sizeof *p);
    for (t = 0; t < NUM_TRACKS; t++) {
        p->len[t] = 16;
        for (s = 0; s < NUM_STEPS; s++) {
            p->s[t][s].vel = 100;
            p->s[t][s].note = 63;
            p->s[t][s].prob = 100;
        }
    }
}

void lxr_initProject(Project *p)
{
    int i, t;
    memset(p, 0, sizeof *p);
    memcpy(p->name, "Project ", NAME_LEN);
    for (i = 0; i < NUM_KITS; i++) {
        char nm[16];
        lxr_initKit(&p->kits[i]);
        snprintf(nm, sizeof nm, "Kit %02d  ", i + 1);
        memcpy(p->kits[i].name, nm, NAME_LEN);
    }
    for (i = 0; i < NUM_PATTERNS; i++) lxr_initPattern(&p->pats[i]);
    for (i = 0; i < NUM_SONGS; i++) {
        char nm[16];
        snprintf(nm, sizeof nm, "Song %02d ", i + 1);
        memcpy(p->songs[i].name, nm, NAME_LEN);
        memset(p->songs[i].e, SONG_END, SONG_LEN);
        p->songs[i].e[0] = 0;
        p->songs[i].e[1] = SONG_LOOP;
    }
    p->g.bpm = 0;              /* follow MPC */
    p->g.mtx = 15;
    p->g.mrx = 15;
    p->g.kcm = 0;
    p->g.roll = 8;            /* 1/16 */
    p->g.quant = 2;
    for (t = 0; t < NUM_TRACKS; t++) { p->g.trackCh[t] = 0; p->g.trackNote[t] = lxr_padNotes[t]; }
    /* every pattern starts empty, as on the machine: the LXR's steps or MPC's own step sequencer write the beat
     * (MPC play runs the LXR's sequencer too, so a pre-filled pattern would play over MPC's) */
}

/* Each voice's MIDI note (MIX page), by default the notes of the Force's first 7 drum pads in its default pad layout:
 * the C major scale from C1 (pad 1 = 36, pad 2 = 38, pad 3 = 40, pad 4 = 41 ... seen on a Force: pads 1-4 played
 * DRUM1, DRUM3, CLP/CYM and CL HH with the voices on 36-42). So pad n plays voice n. */
const uint8_t lxr_padNotes[NUM_TRACKS] = {36, 38, 40, 41, 43, 45, 47};

/* The factory project: the LXR-02 project imported by tools/import_lxr02.py (src/factory_data.c), its kits and
 * patterns, kit change "auto" so each pattern brings the kit it was made with. */
void lxr_factoryProject(Project *p)
{
    int i, k, t;
    lxr_initProject(p);
    memcpy(p->name, "Hrtl DLC", NAME_LEN);
    for (i = 0; i < NUM_KITS; i++) {
        p->kits[i] = lxr_factoryKits[i];
        p->kits[i].b[0] = 0;   /* PAR_NONE: the LXR-02 leaves a stray value here */
        for (t = 0; t < 6; t++) {   /* preset_loadDrumset(): mod targets past the list are off */
            if (p->kits[i].b[PAR_VEL_DEST_1 + t] >= meta_numModTargets()) p->kits[i].b[PAR_VEL_DEST_1 + t] = 0;
            if (p->kits[i].b[PAR_TARGET_LFO1 + t] >= meta_numModTargets()) p->kits[i].b[PAR_TARGET_LFO1 + t] = 0;
        }
        for (k = 1; k < LXR_KIT_BYTES; k++) {
            const uint8_t mx = meta_soundMax((uint16_t)k);
            if (p->kits[i].b[k] > mx) p->kits[i].b[k] = mx;
        }
    }
    /* Both preset sets: kits 1-16 are the Hrtl LXR-02 DLC as the LXR-02 plays them (new), kits 17-32 the same 16 as
     * LXRXXL 0.5.x shipped them (old, "05"), kit 33 LXRXXL 0.5.x's init kit, kit 34 the LXR-02 firmware's init kit;
     * the DLC's empty slots after that keep its Initkit. */
    for (i = 0; i < 16; i++) {
        Kit *d = &p->kits[16 + i];
        int n;
        lxr_kitFromOld(d, lxr_factoryKits05[i]);
        for (n = NAME_LEN; n > 0 && d->name[n - 1] == ' '; n--) {}
        if (n > NAME_LEN - 2) n = NAME_LEN - 2;
        d->name[n] = '0'; d->name[n + 1] = '5';
        for (n += 2; n < NAME_LEN; n++) d->name[n] = ' ';
    }
    initKit05(&p->kits[32]);
    lxr_initKit(&p->kits[33]);
    memcpy(p->kits[33].name, "Initkit ", NAME_LEN);
    for (i = 0; i < lxr_numFactoryPatterns; i++) {
        const LxrFactoryPattern *f = &lxr_factoryPatterns[i];
        Pattern *d = &p->pats[f->pat];
        memcpy(d->s, f->s, sizeof d->s);
        for (t = 0; t < NUM_TRACKS; t++) d->len[t] = f->len[t] >= 1 && f->len[t] <= NUM_STEPS ? f->len[t] : 16;
        d->kit = f->kit < NUM_KITS ? f->kit : 0;
    }
    for (i = 0; i < NUM_PATTERNS; i++) {   /* the empty ones: kit 1, like the machine's empty slots */
        int any = 0;
        for (t = 0; t < NUM_TRACKS && !any; t++) for (k = 0; k < NUM_STEPS; k++) if (p->pats[i].s[t][k].on) { any = 1; break; }
        if (!any) p->pats[i].kit = 0;
    }
    p->g.kcm = 1;
}

/* everything back to the factory project, pattern 1 with its kit (the RESET key, and a new instance) */
void lxr_loadFactory(Lxr *x)
{
    seq_stop(x);
    lxr_dsp_automationReset();
    lxr_factoryProject(&x->p);
    lxr_dsp_silenceFx();
    x->morphLoaded = 0;
    x->morph = 0;
    x->seq.mutes = 0;
    x->seq.rolls = 0;
    x->seq.nextPat = 0xff;
    x->kitSlot = x->p.pats[0].kit;
    x->kit = x->p.kits[x->kitSlot];
    x->patSlot = 0;
    x->pat = x->p.pats[0];
    x->songSlot = 0;
    lxr_applyKit(x);
    x->displayRev++;
}

/* ---------------------------------------------------------------- kit edits */
void lxr_applyKit(Lxr *x)
{
    int i;
    lxr_dsp_applyKit(x->kit.b, x->morphLoaded ? x->morphKit.b : NULL, x->morph);
    x->displayRev++;
}

void lxr_setKitParam(Lxr *x, uint16_t par, uint8_t value, int record)
{
    if (par >= LXR_KIT_BYTES) return;
    x->kit.b[par] = value;
    if (x->morphLoaded && x->morph) lxr_dsp_setParam(par, lxr_dsp_morphValue(x->kit.b, x->morphKit.b, par, x->morph));
    else lxr_dsp_setParam(par, value);
    if (record) seq_recordParam(x, par, value);
    x->dirtyKit = 1;
    x->displayRev++;
}

void lxr_setExt(Lxr *x, uint8_t ext, uint8_t value)
{
    if (ext >= LXR_NUM_EXT) return;
    x->kit.b[LXR_NUM_SOUND_PARAMS + ext] = value;
    if (x->morphLoaded && x->morph)
        lxr_dsp_setExt(ext, lxr_dsp_morphValue(x->kit.b, x->morphKit.b, (uint16_t)(LXR_NUM_SOUND_PARAMS + ext), x->morph));
    else lxr_dsp_setExt(ext, value);
    x->dirtyKit = 1;
    x->displayRev++;
}

/* ---------------------------------------------------------------- notes */
void lxr_trigger(Lxr *x, uint8_t track, uint8_t vel, uint8_t note, int fromUser)
{
    if (track >= NUM_TRACKS) return;
    lxr_dsp_trigger(track, vel, note);
    x->ui.pulse[track] = lxr_clock(x) + 2205u;   /* the voice LED pulses for 50 ms */
    if (fromUser && x->seq.recording) seq_recordNote(x, track, vel, note);
}

/* ---------------------------------------------------------------- render */
uint32_t lxr_clock(Lxr *x) { return x->clock; }

/* the faders' taper, LXRXXL's since 0.1 (LXR02Open's log fader curve, SLIDER_LOG_TAPER_DB 30): a dB-linear curve
 * normalised to 0..1, for the six voice faders and the master. (The Erica firmware squares the position.) */
static float taper(float v)
{
    const float minGain = 0.0316227766f;   /* 10^(-30/20) */
    if (v <= 0.f) return 0.f;
    if (v >= 1.f) return 1.f;
    return (powf(10.f, (v - 1.f) * 1.5f) - minGain) / (1.f - minGain);
}

void lxr_render(Lxr *x, float *L, float *R, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        if (x->outPos >= LXR_BLOCK) {
            int k;
            float g[6];
            const float master = taper(x->fader[6]);
            for (k = 0; k < 6; k++) g[k] = taper(x->fader[k]);
            seq_block(x, LXR_BLOCK);
            lxr_dsp_render(x->outL, x->outR, g);
            for (k = 0; k < LXR_BLOCK; k++) { x->outL[k] *= master; x->outR[k] *= master; }
            x->outPos = 0;
        }
        L[i] = x->outL[x->outPos];
        R[i] = x->outR[x->outPos];
        x->outPos++;
        x->clock++;
    }
}

void lxr_init(Lxr *x, const char *dataDir)
{
    int i;
    memset(x, 0, sizeof *x);
    snprintf(x->dataDir, sizeof x->dataDir, "%s", dataDir ? dataDir : "");
    lxr_factoryProject(&x->p);   /* a new LXRXXL starts on the factory project (MPC's project, if any, replaces it) */
    x->kitSlot = x->p.pats[0].kit;
    x->kit = x->p.kits[x->kitSlot];
    x->pat = x->p.pats[0];
    x->seq.nextPat = 0xff;
    for (i = 0; i < 6; i++) x->fader[i] = 1.f;
    x->fader[6] = 0.8f;
    x->padLearned = 0;   /* nothing assumed about the Force's pad notes: learned once (CONFIG > lrn) */
    x->padBase = 0;
    x->padCol = 1;
    x->padRow = 8;
    x->outPos = LXR_BLOCK;
    lxr_dsp_init();
    lxr_applyKit(x);
    ui_init(x);
    x->rnd.amt = 5;      /* the mod's knobs at 50 % */
    x->rnd.seqAmt = 5;
    store_loadSettings(x);
}

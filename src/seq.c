/* LXRXXL -- the LXR-02 sequencer (owner's manual ch. 6 and 7): 64 patterns of 7 tracks x 64 steps in 4 bars, each track
 * with its own length, step velocity / note / probability and two automation values, manual rolls, shuffle, pattern
 * chaining from the pattern menu, songs, recording and the euclidean pattern generator.
 *
 * Timing: positions are 96 ppq ticks. The engine runs in the firmware's 64-frame blocks and everything that falls inside a
 * block fires at its start, as on the machine (voice triggers are applied at the DSP block boundary there too). With the
 * tempo on "MPC" the position is MPC's song position (ppqPos), so steps sit on MPC's grid: no clock pulses are counted
 * (mpc-vst-plugins docs/MIDI_TIMING.md). */
#include <string.h>
#include <math.h>
#include "lxr.h"

#define TICKS_16TH 24.0
#define ROLL_VOLUME 100

/* manual roll rates (the 0.37 list: One, 1/1 .. 1/128) as 96 ppq ticks; One fires once on the press */
static const uint16_t rollTicks[14] = {0, 384, 192, 128, 96, 64, 48, 32, 24, 16, 12, 8, 6, 3};

static uint32_t rng_state = 0x1234567u;
static uint32_t rnd(void) { rng_state = rng_state * 1664525u + 1013904223u; return rng_state >> 8; }

uint8_t seq_patternLength(const Pattern *p)
{
    uint8_t t, m = 1;
    for (t = 0; t < NUM_TRACKS; t++)
        if (p->len[t] > m) m = p->len[t];
    return m;
}

/* the shuffle (sequencer.c seq_calcDeltaT): every second 16th is late by up to 16 of 96 ticks */
static double shuffle_ticks(Lxr *x, uint32_t n)
{
    if (!(n & 1u)) return 0.0;
    return (x->p.g.shuffle / 127.0) * 16.0;
}

static double step_tick(Lxr *x, uint32_t n) { return n * TICKS_16TH + shuffle_ticks(x, n); }

static SeqRun *run_of(Lxr *x) { return &x->run; }

void seq_loadPattern(Lxr *x, uint8_t pat)
{
    if (pat >= NUM_PATTERNS) return;
    x->patSlot = pat;
    memcpy(&x->pat, &x->p.pats[pat], sizeof(Pattern));
    if (x->p.g.kcm && x->pat.kit < NUM_KITS) {
        x->kitSlot = x->pat.kit;
        x->kit = x->p.kits[x->pat.kit];
        lxr_applyKit(x);
    }
    x->displayRev++;
}

void seq_queuePattern(Lxr *x, uint8_t pat)
{
    if (pat >= NUM_PATTERNS) return;
    if (!x->seq.running) { seq_loadPattern(x, pat); x->seq.nextPat = 0xff; return; }
    x->seq.nextPat = pat;
}

static void song_first(Lxr *x)
{
    const Song *s = &x->p.songs[x->songSlot];
    x->seq.songPos = 0;
    if (s->e[0] < NUM_PATTERNS) seq_loadPattern(x, s->e[0]);
}

/* the next pattern at a pattern's end: the song's next row, or the one the pattern menu queued */
static int pattern_end(Lxr *x)
{
    if (x->seq.songMode) {
        const Song *s = &x->p.songs[x->songSlot];
        uint8_t pos = (uint8_t)(x->seq.songPos + 1);
        uint8_t e = pos < SONG_LEN ? s->e[pos] : SONG_END;
        if (e == SONG_LOOP) { pos = 0; e = s->e[0]; }
        if (e >= NUM_PATTERNS) {   /* END: the song stops */
            seq_stop(x);
            return 0;
        }
        x->seq.songPos = pos;
        seq_loadPattern(x, e);
        return 1;
    }
    if (x->seq.nextPat != 0xff) {
        seq_loadPattern(x, x->seq.nextPat);
        x->seq.nextPat = 0xff;
        return 1;
    }
    return 1;
}

static void fire_step(Lxr *x, SeqRun *r, uint32_t n)
{
    uint8_t t;
    uint32_t rel;
    const uint8_t patLen = seq_patternLength(&x->pat);
    if (n > r->patStart && (n - r->patStart) % patLen == 0) {
        if (!pattern_end(x)) return;
        r->patStart = n;
    }
    rel = n - r->patStart;
    x->seq.masterPos = (uint16_t)(rel % seq_patternLength(&x->pat));
    x->seq.beatLed = (n % 4u) == 0;
    for (t = 0; t < NUM_TRACKS; t++) {
        const uint8_t len = x->pat.len[t] ? x->pat.len[t] : 16;
        const uint8_t s = (uint8_t)(rel % len);
        Step *st = &x->pat.s[t][s];
        x->seq.lastPos[t] = s;
        if (x->seq.erasing && t == x->ui.track) {
            st->on = 0;
            continue;
        }
        if (r->skipNext[t]) { r->skipNext[t] = 0; continue; }
        if (!st->on || (x->seq.mutes & (1u << t))) continue;
        if (st->prob < 100 && (rnd() % 100u) >= st->prob) continue;
        lxr_dsp_automation(t, 0, st->p1, st->v1);
        lxr_dsp_automation(t, 1, st->p2, st->v2);
        lxr_trigger(x, t, st->vel, st->note, 0);
    }
    x->displayRev++;
}

static void fire_rolls(Lxr *x, SeqRun *r, double t0, double t1)
{
    uint8_t t;
    const uint16_t per = rollTicks[x->p.g.roll < 14 ? x->p.g.roll : 8];
    if (!x->seq.rolls || per == 0) return;
    for (t = 0; t < NUM_TRACKS; t++) {
        double k;
        if (!(x->seq.rolls & (1u << t))) continue;
        k = ceil(t0 / per) * per;
        if (k < t1 && k != r->rollMark[t]) {
            const uint8_t s = x->seq.lastPos[t];
            r->rollMark[t] = k;
            lxr_trigger(x, t, ROLL_VOLUME, x->pat.s[t][s].note ? x->pat.s[t][s].note : 63, 0);
            if (x->seq.recording) seq_recordNote(x, t, ROLL_VOLUME, 63);
        }
    }
}

static void begin_at(Lxr *x, double tick)
{
    SeqRun *r = run_of(x);
    const uint8_t patLen = seq_patternLength(&x->pat);
    uint8_t t;
    r->pos = tick;
    r->n = (uint32_t)ceil(tick / TICKS_16TH - 1e-9);
    /* MPC's grid: the pattern runs as if it had started at the song's beginning, so bar 1 of MPC is step 1 */
    r->patStart = r->n - (r->n % patLen);
    for (t = 0; t < NUM_TRACKS; t++) { r->rollMark[t] = -1; r->skipNext[t] = 0; }
}

void seq_start(Lxr *x, int song)
{
    x->seq.songMode = (uint8_t)(song != 0);
    if (song) song_first(x);
    x->seq.running = 1;
    x->seq.freeRun = (uint8_t)!(x->p.g.bpm == 0 && x->seq.hostValid && x->seq.hostPlaying);
    lxr_dsp_automationReset();
    if (!x->seq.freeRun) begin_at(x, x->seq.hostPpq * 96.0);
    else begin_at(x, 0.0);
    x->displayRev++;
}

void seq_stop(Lxr *x)
{
    x->seq.running = 0;
    x->seq.freeRun = 0;
    x->seq.songMode = 0;
    x->seq.erasing = 0;
    x->seq.beatLed = 0;
    x->displayRev++;
}

/* MPC's transport and tempo, once per host block (engine.c) */
void seq_hostTime(Lxr *x, int playing, double ppq, double tempo)
{
    const int wasPlaying = x->seq.hostPlaying;
    x->seq.hostValid = 1;
    if (tempo > 0) x->seq.hostTempo = tempo;
    x->seq.hostPpq = ppq;
    x->seq.hostPlaying = (uint8_t)(playing != 0);
    x->seq.frames = 0;
    if (x->p.g.bpm != 0) return;   /* the LXR runs on its own tempo */
    if (playing && !wasPlaying) {   /* MPC plays: it takes over (song mode armed with SHIFT + PLAY stays) */
        const int song = x->seq.songMode;
        if (x->seq.running) seq_stop(x);
        seq_start(x, song);
    }
    else if (!playing && wasPlaying) { if (!x->seq.freeRun) seq_stop(x); }
    else if (playing && x->seq.running && !x->seq.freeRun) {
        SeqRun *r = run_of(x);
        /* a loop or a locate: back to MPC's position */
        if (ppq * 96.0 < r->pos - 1.0 || ppq * 96.0 > r->pos + 96.0) begin_at(x, ppq * 96.0);
    }
}

void seq_block(Lxr *x, int frames)
{
    SeqRun *r;
    double t0, t1;
    const int follow = x->p.g.bpm == 0;
    const double bpm = follow ? (x->seq.hostTempo > 0 ? x->seq.hostTempo : 120.0) : x->p.g.bpm;
    if (x->seq.bpm != (float)bpm) { x->seq.bpm = (float)bpm; lxr_dsp_setTempo((float)bpm); }
    if (!x->seq.running) return;
    r = run_of(x);
    t0 = r->pos;
    if (follow && x->seq.hostValid && !x->seq.freeRun)
        t1 = (x->seq.hostPpq + (x->seq.frames + frames) * bpm / 60.0 / 44100.0) * 96.0;
    else
        t1 = t0 + frames * bpm / 60.0 / 44100.0 * 96.0;
    x->seq.frames += (uint32_t)frames;
    if (t1 < t0) t1 = t0;
    while (step_tick(x, r->n) < t1 && x->seq.running) {
        fire_step(x, r, r->n);
        r->n++;
    }
    fire_rolls(x, r, t0, t1);
    r->pos = t1;
    x->seq.tick = t1;
    x->seq.sixteenth = r->n;
}

/* ---------------------------------------------------------------- recording */
static uint8_t quant_step(Lxr *x, uint8_t track, int *upcoming)
{
    SeqRun *r = run_of(x);
    const uint8_t len = x->pat.len[track] ? x->pat.len[track] : 16;
    /* the nearest 16th (the firmware quantises recorded notes to its quantisation grid; 16ths here) */
    uint32_t nq = (uint32_t)floor(r->pos / TICKS_16TH + 0.5);
    *upcoming = nq >= r->n;
    if (nq < r->patStart) nq = r->patStart;
    return (uint8_t)((nq - r->patStart) % len);
}

void seq_recordNote(Lxr *x, uint8_t track, uint8_t vel, uint8_t note)
{
    int upcoming;
    uint8_t s;
    Step *st;
    if (!x->seq.running || !x->seq.recording || track >= NUM_TRACKS) return;
    s = quant_step(x, track, &upcoming);
    st = &x->pat.s[track][s];
    st->on = 1;
    st->vel = vel ? vel : 1;
    st->note = note;
    st->prob = 100;
    if (upcoming) run_of(x)->skipNext[track] = 1;   /* it sounded now: don't play it twice */
    x->displayRev++;
}

void seq_recordParam(Lxr *x, uint16_t par, uint8_t value)
{
    Step *st;
    if (!x->seq.running || !x->seq.recording) return;
    st = &x->pat.s[x->ui.track][x->seq.lastPos[x->ui.track]];
    if (x->p.g.recTrack == 0) { st->p1 = (uint8_t)(par + 1); st->v1 = value; }
    else { st->p2 = (uint8_t)(par + 1); st->v2 = value; }
}

/* ---------------------------------------------------------------- euclidean generator (7.5) */
void seq_euclid(Lxr *x, uint8_t track)
{
    const uint8_t len = x->ui.genLen[track] < 1 ? 1 : x->ui.genLen[track] > 64 ? 64 : x->ui.genLen[track];
    const uint8_t steps = x->ui.genSteps[track] > len ? len : x->ui.genSteps[track];
    const uint8_t rot = x->ui.genRot[track];
    uint8_t i;
    x->pat.len[track] = len;
    for (i = 0; i < NUM_STEPS; i++) x->pat.s[track][i].on = 0;
    for (i = 0; i < len; i++) {
        /* Bresenham's form of the euclidean rhythm: a pulse where the running sum wraps */
        const uint8_t src = (uint8_t)((i + len - (rot % len)) % len);
        if (steps && ((src * steps) % len) < steps) {
            Step *st = &x->pat.s[track][i];
            st->on = 1;
            if (!st->vel) st->vel = 100;
            if (!st->note) st->note = 63;
            if (st->prob == 0) st->prob = 100;
        }
    }
    x->displayRev++;
}

void seq_clearTrackAt(Lxr *x, uint8_t track)
{
    Step *st = &x->pat.s[track][x->seq.lastPos[track]];
    st->on = 0;
}

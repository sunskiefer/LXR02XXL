/* LXRXXL offline engine tests (x86, run under valgrind by test/run_tests.sh): every voice sounds, the sequencer keeps
 * time on its own tempo and on MPC's, the front panel's keys do what the manual says, the state chunk round-trips, two
 * instances don't share engine state, and a long random walk over every control never crashes or goes NaN. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "engine.h"
#include "lxr.h"
#include "lxr_state.h"
#include "param_ids.h"
#include "ParameterArray.h"
#include "menu.h"

typedef struct { Lxr x; LxrState *st; } Inst;   /* engine.c's instance: the Lxr first */

static const mpc_engine_t *E;
static int fails;
#define CHECK(c, ...) do { printf("%s ", (c) ? "ok  " : "FAIL"); printf(__VA_ARGS__); printf("\n"); if (!(c)) fails++; } while (0)

static double render(Inst *in, int frames, int16_t *keep)
{
    int16_t b[256];
    double e = 0;
    int done = 0;
    while (done < frames) {
        int n = frames - done > 128 ? 128 : frames - done, i;
        E->render(in, b, n);
        for (i = 0; i < 2 * n; i++) e += (double)b[i] * b[i];
        if (keep) memcpy(keep + 2 * done, b, sizeof(int16_t) * 2 * n);
        done += n;
    }
    return sqrt(e / (2.0 * frames)) / 32768.0;
}

static void press(Inst *in, const char *key) { E->set_param(in, key, "1"); }
static int led(Inst *in, const char *key)
{
    char k[32], b[8] = "";
    snprintf(k, sizeof k, "%s_on", key);
    E->get_param(in, k, b, sizeof b);
    return b[0] == '1';
}
static void lcd(Inst *in, char *l1, char *l2)
{
    E->get_param(in, "lcd1", l1, 48);
    E->get_param(in, "lcd2", l2, 48);
}
static void trig(Inst *in, int t)
{
    lxr_state_enter(in->st);
    lxr_trigger(&in->x, (uint8_t)t, 110, 63, 0);
    lxr_state_leave();
}
static void stopAll(Inst *in)
{
    lxr_state_enter(in->st);
    seq_stop(&in->x);
    lxr_state_leave();
}

static void voices(void)
{
    Inst *in = E->create(NULL);
    static const char *names[7] = {"DRUM1", "DRUM2", "DRUM3", "SNARE", "CLP/CYM", "CL HH", "OP HH"};
    int t;
    stopAll(in);
    CHECK(render(in, 4410, NULL) < 1e-4, "silent before any trigger");
    for (t = 0; t < 7; t++) {
        double r;
        static int16_t buf[2 * 8820];
        int i, pk = 0;
        trig(in, t);
        r = render(in, 8820, buf);
        for (i = 0; i < 2 * 8820; i++) if (abs(buf[i]) > pk) pk = abs(buf[i]);
        CHECK(r > 0.002 && isfinite(r), "%-8s sounds (rms %.4f, peak %.2f)", names[t], r, pk / 32768.0);
        render(in, 44100, NULL);   /* let it die away */
    }
    E->destroy(in);
}

static void sequencer(void)
{
    Inst *in = E->create(NULL);
    Lxr *x = &in->x;
    uint32_t n0;
    /* its own tempo: 120 BPM is 8 16ths a second */
    x->p.g.bpm = 120;
    press(in, "play");
    CHECK(x->seq.running, "PLAY starts the sequencer");
    render(in, 1200, NULL);   /* past the tap resync */
    CHECK(led(in, "play") == 1, "PLAY lit while running");
    n0 = x->run.n;
    render(in, 44100 * 2, NULL);
    CHECK(x->run.n - n0 >= 16 && x->run.n - n0 <= 17, "120 BPM: 16 steps in 2 s (%u)", x->run.n - n0);
    press(in, "play");
    CHECK(!x->seq.running, "PLAY again stops it");

    /* MPC's tempo and transport (bpm = MPC): steps follow ppqPos */
    x->p.g.bpm = 0;
    E->set_param(in, "host_time", "1 4.0 100.0");
    CHECK(x->seq.running, "follows MPC's play");
    CHECK(x->run.n == 16, "starts at MPC's position (beat 4 = 16th 16: %u)", x->run.n);
    {
        int b;
        double ppq = 4.0;
        for (b = 0; b < 200; b++) {   /* 200 host blocks of 256 frames at 100 BPM */
            char s[64];
            snprintf(s, sizeof s, "1 %.9f 100.0", ppq);
            E->set_param(in, "host_time", s);
            render(in, 256, NULL);
            ppq += 256.0 * 100.0 / 60.0 / 44100.0;
        }
        CHECK(fabs(x->run.pos / 96.0 - ppq) < 0.01, "stays on MPC's position after 200 blocks (%.4f vs %.4f)", x->run.pos / 96.0, ppq);
        CHECK(x->seq.bpm == 100.0f, "uses MPC's tempo");
        E->set_param(in, "host_time", "1 0.0 100.0");   /* a loop back to the start */
        render(in, 64, NULL);
        CHECK(x->run.n <= 1, "a loop jumps back with MPC (%u)", x->run.n);
        E->set_param(in, "host_time", "0 0.0 100.0");
        render(in, 64, NULL);
        CHECK(!x->seq.running, "stops with MPC");
    }
    E->destroy(in);
}

static void panel(void)
{
    Inst *in = E->create(NULL);
    Lxr *x = &in->x;
    char l1[48], l2[48];
    int i, on;
    stopAll(in);
    /* voice select + step keys: a step toggles, its LED follows (red = on) */
    press(in, "v2");
    CHECK(x->ui.track == 1, "DRUM2 selects track 2");
    on = x->pat.s[1][4].on;
    press(in, "s5");
    CHECK(x->pat.s[1][4].on == !on, "step 5 toggles on track 2");
    render(in, 1200, NULL);   /* past the tap resync */
    CHECK(led(in, "s5") == x->pat.s[1][4].on, "step 5 LED shows the step");
    /* menu sections fill the screen's sliders with the page's values */
    press(in, "filter");
    lcd(in, l1, l2);
    CHECK(x->ui.section == 5 && ui_slotUsed(x, 0), "FILTER page: LCD '%s' / '%s'", l1, l2);
    {
        char nm[32];
        E->get_param(in, "slot1_name", nm, sizeof nm);
        CHECK(nm[0] && nm[0] != ' ', "slider 1 is named from the page ('%s')", nm);
    }
    /* a slider edits the kit */
    {
        uint8_t before[LXR_KIT_BYTES];
        memcpy(before, x->kit.b, sizeof before);
        E->set_param(in, "slot1", "0.9");
        CHECK(memcmp(before, x->kit.b, sizeof before) != 0, "slider 1 changes the kit");
    }
    /* SHIFT latches; SHIFT + step 12 = CONFIG */
    press(in, "shift");
    CHECK(x->ui.shift, "SHIFT latches on a tap");
    press(in, "s12");
    CHECK(x->ui.mode == M_CONFIG, "SHIFT + 12 opens CONFIG");
    lcd(in, l1, l2);
    printf("     CONFIG: '%s' / '%s'\n", l1, l2);
    press(in, "voice");
    CHECK(x->ui.mode == M_VOICE, "VOICE returns to the voice page");
    /* PERF: the voice keys mute, the step keys 1-7 roll */
    press(in, "perf");
    press(in, "v3");
    CHECK(x->seq.mutes & 4, "PERF + DRUM3 mutes DRUM3");
    press(in, "v3");
    CHECK(!(x->seq.mutes & 4), "again unmutes");
    press(in, "s4");
    CHECK(x->seq.rolls & 8, "PERF + step 4 rolls the snare");
    press(in, "s4");
    CHECK(!(x->seq.rolls & 8), "again stops the roll");
    press(in, "voice");
    /* the data encoder and the pots */
    press(in, "osc");
    for (i = 0; i < 4; i++) E->set_param(in, "data", "0.5078125");
    E->set_param(in, "pot1", "0.4921875");
    lcd(in, l1, l2);
    CHECK(1, "DATA and pots move the selection/values: '%s' / '%s'", l1, l2);
    E->destroy(in);
}

static void chunk(void)
{
    Inst *a = E->create(NULL), *b = E->create(NULL);
    char *s;
    int n;
    a->x.pat.s[3][9].on = 1; a->x.pat.s[3][9].vel = 77; a->x.pat.len[3] = 48;
    a->x.p.pats[5].s[0][63].on = 1;
    a->x.kit.b[10] ^= 0x55;
    a->x.fader[2] = 0.25f;
    n = E->get_param(a, "state", NULL, 0);
    s = malloc((size_t)n + 1);
    E->get_param(a, "state", s, n + 1);
    CHECK(n > 0 && n < 64 * 1024, "state chunk is %d bytes", n);
    E->set_param(b, "state", s);
    CHECK(!memcmp(&a->x.pat, &b->x.pat, sizeof(Pattern)), "working pattern restored");
    CHECK(!memcmp(&a->x.p, &b->x.p, sizeof(Project)), "whole project restored");
    CHECK(!memcmp(&a->x.kit, &b->x.kit, sizeof(Kit)), "kit restored");
    CHECK(fabsf(b->x.fader[2] - 0.25f) < 0.01f, "faders restored");
    free(s);
    E->destroy(a);
    E->destroy(b);
}

static void isolation(void)
{
    enum { N = 44100 };
    static int16_t ref[2 * N], got[2 * N];
    Inst *a, *b;
    int k;
    /* reference: b alone, hats + snare (noise and the RNG) */
    b = E->create(NULL);
    stopAll(b);
    trig(b, 3); trig(b, 6);
    render(b, N, ref);
    E->destroy(b);
    /* again, with another instance playing something else in between every block */
    a = E->create(NULL);
    b = E->create(NULL);
    stopAll(a); stopAll(b);
    lxr_state_enter(a->st); lxr_setKitParam(&a->x, 2, 127, 0); lxr_state_leave();
    trig(b, 3); trig(b, 6);
    for (k = 0; k < N; k += 128) {
        trig(a, (k / 128) % 7);
        render(a, 128, NULL);
        render(b, 128 > N - k ? N - k : 128, got + 2 * k);
    }
    CHECK(!memcmp(ref, got, sizeof ref), "an instance renders the same with another one playing beside it");
    E->destroy(a);
    E->destroy(b);
}

static void fuzz(void)
{
    Inst *in = E->create(NULL);
    unsigned r = 12345;
    int i, bad = 0;
    char v[32], l1[48], l2[48], t[64];
    for (i = 0; i < 20000; i++) {
        int p;
        r = r * 1103515245u + 12345u;
        p = (int)((r >> 8) % P_COUNT);
        r = r * 1103515245u + 12345u;
        snprintf(v, sizeof v, "%.4f", ((r >> 8) % 10001) / 10000.0);
        if (p == P_DATA || (p >= P_POT1 && p <= P_POT4)) snprintf(v, sizeof v, "%.7f", 0.5 + (((int)(r >> 8) % 9) - 4) / 128.0);
        E->set_param(in, PARAM_KEYS[p], v);
        if (i % 7 == 0) { double e = render(in, 97, NULL); if (!isfinite(e)) bad++; }
        if (i % 13 == 0) lcd(in, l1, l2);
        if (i % 17 == 0) { snprintf(t, sizeof t, "slot%d_display", (int)(r % 8) + 1); E->get_param(in, t, l1, 48); }
        if (i % 19 == 0) { uint8_t m[3] = {0x90, (uint8_t)(36 + r % 40), (uint8_t)(r % 128)}; E->midi(in, m, 3); }
        if (i % 23 == 0) { uint8_t m[3] = {0xB0, (uint8_t)(r % 128), (uint8_t)((r >> 9) % 128)}; E->midi(in, m, 3); }
    }
    {
        double e = render(in, 44100, NULL);
        CHECK(!bad && isfinite(e), "20000 random control moves, notes and CCs: no NaN (final rms %.4f)", e);
    }
    E->destroy(in);
}

/* the mod's randomizers */
static void randomizers(void)
{
    Inst *in = E->create(NULL);
    Lxr *x = &in->x;
    static const int vols[6] = {PAR_VOL1, PAR_VOL2, PAR_VOL3, PAR_VOL4, PAR_VOL5, PAR_VOL6};
    static const int decs[7] = {PAR_VELOD1, PAR_VELOD2, PAR_VELOD3, PAR_VELOD4, PAR_VELOD5, PAR_VELOD6_CLOSED, PAR_VELOD6_OPEN};
    static const int coarse[6] = {PAR_COARSE1, PAR_COARSE2, PAR_COARSE3, PAR_COARSE4, PAR_COARSE5, PAR_COARSE6};
    static const int ff[6] = {PAR_FILTER_FREQ_1, PAR_FILTER_FREQ_2, PAR_FILTER_FREQ_3, PAR_FILTER_FREQ_4, PAR_FILTER_FREQ_5, PAR_FILTER_FREQ_6};
    static const int ft[6] = {PAR_FILTER_TYPE_1, PAR_FILTER_TYPE_2, PAR_FILTER_TYPE_3, PAR_FILTER_TYPE_4, PAR_FILTER_TYPE_5, PAR_FILTER_TYPE_6};
    uint8_t before[LXR_KIT_BYTES], before2[LXR_KIT_BYTES];
    int pr, i, floorsOk = 1, repeats = 0, moved = 0, silentVoices = 0, sameAll = 0, up = 0, down = 0;
    stopAll(in);
    memcpy(before2, x->kit.b, sizeof before2);
    for (pr = 0; pr < 40; pr++) {
        memcpy(before, x->kit.b, sizeof before);
        x->rnd.amt = (uint8_t)(pr % 2 ? 10 : 3);   /* 100 % and 30 % */
        press(in, "rnd");
        for (i = 1; i < END_OF_SOUND_PARAMETERS; i++) {
            if (x->kit.b[i] != before[i]) moved++;
            if (x->kit.b[i] > before[i]) up++;
            if (x->kit.b[i] < before[i]) down++;
            if (x->kit.b[i] != before[i] && x->kit.b[i] == before2[i]) repeats++;
        }
        for (i = 0; i < 6; i++) {
            if (x->kit.b[vols[i]] < 38 || x->kit.b[coarse[i]] < 38) floorsOk = 0;
            if (x->kit.b[ft[i]] == 1 ? x->kit.b[ff[i]] > 88 : x->kit.b[ff[i]] < 38) floorsOk = 0;
        }
        for (i = 0; i < 7; i++) if (x->kit.b[decs[i]] < 38) floorsOk = 0;
        if (!memcmp(before, x->kit.b, sizeof before)) sameAll++;
        memcpy(before2, before, sizeof before2);
        if (pr % 8 == 7) {   /* every voice still sounds */
            int t;
            for (t = 0; t < 7; t++) {
                double r;
                trig(in, t);
                r = render(in, 44100, NULL);   /* a second: an attack may be up to 70 % long */
                if (!(r > 0.0003) || !isfinite(r)) { silentVoices++; printf("     press %d: voice %d rms %.5f\n", pr, t, r); }
                render(in, 22050, NULL);
            }
        }
    }
    CHECK(floorsOk, "40 presses: volumes, decays, pitch and cutoffs stay >= 30 %% (HP cutoff <= 70 %%)");
    CHECK(sameAll == 0 && moved > 40 * 100, "every press moves the voices (%d moves)", moved);
    CHECK(repeats == 0, "no parameter lands back where it was before the last press (%d)", repeats);
    CHECK(up > moved / 3 && down > moved / 3, "it goes up and down (%d up, %d down)", up, down);
    CHECK(silentVoices == 0, "all 7 voices keep sounding (%d silent)", silentVoices);
    /* the step randomizer: density follows the knob, varies per voice, never repeats */
    {
        int t, n10 = 0, n100 = 0, diff = 0, k;
        uint8_t prev[NUM_TRACKS][NUM_STEPS];
        x->rnd.seqAmt = 1;
        press(in, "srnd");
        for (t = 0; t < NUM_TRACKS; t++) for (k = 0; k < NUM_STEPS; k++) n10 += x->pat.s[t][k].on;
        x->rnd.seqAmt = 10;
        press(in, "srnd");
        for (t = 0; t < NUM_TRACKS; t++) for (k = 0; k < NUM_STEPS; k++) n100 += x->pat.s[t][k].on;
        CHECK(n10 >= 7 && n10 <= 21 && n100 > 50, "10 %% gives %d hits over 7 voices, 100 %% gives %d", n10, n100);
        x->rnd.seqAmt = 4;
        for (k = 0; k < 10; k++) {
            for (t = 0; t < NUM_TRACKS; t++) for (i = 0; i < NUM_STEPS; i++) prev[t][i] = x->pat.s[t][i].on;
            press(in, "srnd");
            for (t = 0; t < NUM_TRACKS; t++) {
                int same = 1;
                for (i = 0; i < NUM_STEPS; i++) if (prev[t][i] != x->pat.s[t][i].on) same = 0;
                diff += !same;
            }
        }
        CHECK(diff == 70, "every press gives every voice a new beat (%d / 70)", diff);
    }
    E->destroy(in);
}

/* the factory project, RESET, and SHIFT + the randomizers on one voice */
static void factory(void)
{
    Inst *in = E->create(NULL);
    Lxr *x = &in->x;
    int t, s, n = 0, other = 0;
    uint8_t before[LXR_KIT_BYTES];
    for (t = 0; t < NUM_TRACKS; t++) for (s = 0; s < NUM_STEPS; s++) n += x->pat.s[t][s].on;
    CHECK(!memcmp(x->p.kits[0].name, "Lambo   ", 8) && !memcmp(x->p.kits[15].name, "Mist    ", 8),
          "factory kits: '%.8s' .. '%.8s'", x->p.kits[0].name, x->p.kits[15].name);
    CHECK(n > 20 && x->p.g.kcm == 1 && !memcmp(x->kit.b, x->p.kits[x->p.pats[0].kit].b, LXR_KIT_BYTES),
          "starts on pattern 1 (%d steps) with its kit, kit change auto", n);
    stopAll(in);
    x->kit.b[PAR_VOL1] = 3; x->pat.s[0][1].on ^= 1; x->p.kits[3].name[0] = 'Z';
    press(in, "reset");
    CHECK(x->kit.b[PAR_VOL1] != 3 && x->p.kits[3].name[0] != 'Z' && !memcmp(&x->pat, &x->p.pats[0], sizeof(Pattern)),
          "RESET brings the factory project back");
    /* SHIFT + RANDOM: only the selected voice (DRUM2) */
    press(in, "v2");
    memcpy(before, x->kit.b, sizeof before);
    x->rnd.amt = 10;
    press(in, "shift");
    press(in, "rnd");
    for (s = 1; s < LXR_NUM_SOUND_PARAMS; s++) {
        if (x->kit.b[s] == before[s]) continue;
        if (s == PAR_COARSE2 || s == PAR_FINE2 || s == PAR_VOL2 || s == PAR_FILTER_FREQ_2 || s == PAR_VELOD2 || s == PAR_PAN2) continue;
        if (s == PAR_COARSE1 || s == PAR_VOL1 || s == PAR_VOL4 || s == PAR_FILTER_FREQ_6 || s == PAR_COARSE6) other++;
    }
    CHECK(other == 0 && x->kit.b[PAR_VOL2] != before[PAR_VOL2] && !x->ui.shift,
          "SHIFT + RANDOM changes DRUM2 only (%d others moved), and spends SHIFT", other);
    /* SHIFT + SEQ RND: only the selected track */
    {
        Pattern pb = x->pat;
        int changed = 0, others = 0;
        x->rnd.seqAmt = 5;
        press(in, "shift");
        press(in, "srnd");
        for (t = 0; t < NUM_TRACKS; t++) for (s = 0; s < NUM_STEPS; s++) if (pb.s[t][s].on != x->pat.s[t][s].on) { if (t == 1) changed++; else others++; }
        CHECK(changed > 0 && others == 0, "SHIFT + SEQ RND: a new beat on DRUM2 only (%d changed, %d elsewhere)", changed, others);
    }
    E->destroy(in);
}

/* the LXR-02 firmware engine (0.6): native kits with their own mod targets, every FX type, both preset sets, and
 * MPC projects saved by LXRXXL 0.5.x */
static void lxr02(void)
{
    Inst *in = E->create(NULL);
    Lxr *x = &in->x;
    int k, t, ext = 0, sound = 1;
    char name[9];
    for (k = 0; k < 16; k++)
        for (t = 0; t < 6; t++) {
            const uint16_t a = modTargets[x->p.kits[k].b[PAR_TARGET_LFO1 + t]].param, b = modTargets[x->p.kits[k].b[PAR_VEL_DEST_1 + t]].param;
            if (a >= LXR_NUM_SOUND_PARAMS || b >= LXR_NUM_SOUND_PARAMS) ext++;
        }
    CHECK(ext > 0, "the DLC kits keep their pulse width / FX modulation targets (%d)", ext);
    memcpy(name, x->p.kits[16].name, 8); name[8] = 0;
    CHECK(!strcmp(name, "Lambo05 ") && !memcmp(x->p.kits[31].name, "Mist05  ", 8) && !memcmp(x->p.kits[32].name, "Init 05 ", 8)
          && !memcmp(x->p.kits[33].name, "Initkit ", 8), "both preset sets: '%s' .. '%.8s', '%.8s', '%.8s'", name,
          x->p.kits[31].name, x->p.kits[32].name, x->p.kits[33].name);
    CHECK(!memcmp(x->p.kits[16].b + 1, x->p.kits[0].b + 1, PAR_VEL_DEST_1 - 1), "the 0.5 kits are the same sounds (Lambo05 = Lambo up to the targets)");
    /* every FX type on the FX bus, a hit, then the tail */
    stopAll(in);
    lxr_state_enter(in->st);
    for (t = 0; t < 6; t++) lxr_setKitParam(x, (uint16_t)(PAR_AUDIO_OUT1 + t), 6, 0);
    lxr_setExt(x, EXT_FX_DW, 100);
    lxr_state_leave();
    for (k = 0; k <= 5; k++) {
        double hit, tail;
        lxr_state_enter(in->st);
        lxr_setExt(x, EXT_FX_TYPE, (uint8_t)k);
        lxr_setExt(x, EXT_FX_DELAYTYPE, (uint8_t)(k == 4));
        lxr_setExt(x, EXT_FX_P1, 80); lxr_setExt(x, EXT_FX_P2, 90); lxr_setExt(x, EXT_FX_P3, 100); lxr_setExt(x, EXT_FX_P4, 100);
        lxr_trigger(x, 0, 120, 63, 0); lxr_trigger(x, 3, 120, 63, 0);
        lxr_state_leave();
        hit = render(in, 4410, NULL);
        render(in, 44100, NULL);
        tail = render(in, 4410, NULL);
        if (!(hit > 0.001 || tail > 0.001) || isnan(hit) || isnan(tail)) sound = 0;
        if (k == 1) CHECK(tail < 0.001, "drive: no DC left on the output once the hit has gone (%.5f)", tail);
        if (k == 5) CHECK(tail > 0.00005 || hit > 0.001, "reverb: hit %.4f, a second later %.5f", hit, tail);
    }
    CHECK(sound, "drive, ring, compressor, ping-pong delay and reverb all sound on the FX bus");
    E->destroy(in);

    /* an MPC project saved with LXRXXL 0.5.10 */
    {
        FILE *f = fopen("../../test/data/state_0510.txt", "r");
        char *txt = calloc(1, 1 << 20);
        Inst *o = E->create(NULL);
        Lxr *y = &o->x;
        if (f && txt) { size_t r = fread(txt, 1, (1 << 20) - 1, f); txt[r] = 0; }
        if (f) fclose(f);
        E->set_param(o, "state", txt ? txt : "");
        CHECK(!memcmp(y->kit.name, "OldKit  ", 8) && y->kit.b[PAR_COARSE1] == 77 && !memcmp(y->p.kits[5].name, "OldKit  ", 8),
              "a 0.5.10 MPC project loads ('%.8s')", y->kit.name);
        CHECK(modTargets[y->kit.b[PAR_TARGET_LFO1]].param == PAR_VELOD1 && modTargets[y->kit.b[PAR_VEL_DEST_1]].param == PAR_FILTER_FREQ_1,
              "its LFO and velocity targets point at the same parameters (%d, %d)", modTargets[y->kit.b[PAR_TARGET_LFO1]].param,
              modTargets[y->kit.b[PAR_VEL_DEST_1]].param);
        CHECK(y->kit.b[PAR_FX_TYPE] == 4 && y->kit.b[PAR_FX_DELAY_TYPE] == 1 && y->kit.b[PAR_FX_REVERB_TYPE] == 0 && y->kit.b[PAR_FX_P2] == 127,
              "its FX unit: ping-pong delay, range high");
        free(txt);
        E->destroy(o);
    }
}

int main(void)
{
    E = mpc_engine();
    voices();
    sequencer();
    panel();
    chunk();
    isolation();
    randomizers();
    factory();
    lxr02();
    fuzz();
    printf(fails ? "FAILED (%d)\n" : "PASSED\n", fails);
    return fails ? 1 : 0;
}

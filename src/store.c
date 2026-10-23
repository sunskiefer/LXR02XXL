/* LXRXXL -- projects on the device's storage, the MPC state chunk and the device-wide settings.
 *
 * A project is one file, <data>/Projects/projNN.lxr: the whole project (64 kits, 64 patterns, 64 songs, the global
 * settings) as the LXR holds it in RAM. "Save project" writes it, "Load project" reads it (7.4); both run on a worker
 * thread so the audio never waits for the card. MPC's own project keeps the instance's whole state (store_serialize):
 * the project plus the working kit, morph kit and pattern, so a saved MPC project reopens exactly as it was left. */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>
#include "lxr.h"

#define PROJ_MAGIC "LXRXXLP2"      /* 2: kits are the LXR-02's 247 bytes (0.6) */
#define PROJ_MAGIC_V1 "LXRXXLP1"   /* LXRXXL 0.5.x: 245-byte kits, the 0.37 engine's mod target numbers */
#define STATE_MAGIC "LXS1"

/* ---------------------------------------------------------------- LXRXXL 0.5.x data
 * Kits were 8 name bytes + 245: the sound parameters 0..227, then pulse widths, the FX unit and the delay type at
 * 244. The velocity and LFO targets were indices into the 0.37 engine's target list. Everything else in a project has
 * the same layout, so an old project is read with the old kit size and its kits moved over. */
#define OLD_KIT_BYTES 245
#define OLD_KIT_SIZE (NAME_LEN + OLD_KIT_BYTES)
#define OLD_PROJECT_SIZE (sizeof(Project) - NUM_KITS * (sizeof(Kit) - OLD_KIT_SIZE))
_Static_assert(sizeof(Kit) == NAME_LEN + LXR_KIT_BYTES, "a Kit has no padding");
_Static_assert(offsetof(Project, kits) == NAME_LEN, "kits follow the name");
#include "legacy_targets.inc"
#include "Parameters.h"

static uint8_t legacy_target(uint8_t old)
{
    const uint8_t par = old < sizeof legacyTargetParam ? legacyTargetParam[old] : 0;
    return par ? meta_paramToModTarget(par) : 0;
}

void lxr_kitFromOld(Kit *k, const unsigned char *old)
{
    int i;
    const uint8_t *ob = old + NAME_LEN;
    memset(k, 0, sizeof *k);
    memcpy(k->name, old, NAME_LEN);
    memcpy(k->b, ob, OLD_KIT_BYTES - 1);                       /* 0..243: same places */
    k->b[0] = 0;                                               /* PAR_NONE (0.5's factory kits kept a stray value) */
    k->b[PAR_FX_DELAY_TYPE] = ob[OLD_KIT_BYTES - 1];           /* was at 244 */
    k->b[PAR_FX_REVERB_TYPE] = 0;
    k->b[PAR_MORPH_KIT_DEST] = 0;
    for (i = 0; i < 6; i++) {
        k->b[PAR_VEL_DEST_1 + i] = legacy_target(ob[PAR_VEL_DEST_1 + i]);
        k->b[PAR_TARGET_LFO1 + i] = legacy_target(ob[PAR_TARGET_LFO1 + i]);
    }
    if (k->b[PAR_FX_DRY_WET] > 100) k->b[PAR_FX_DRY_WET] = 100;   /* the LXR-02's dry/wet is 0..100 */
    if (k->b[PAR_FX_TYPE] == 4 && k->b[PAR_FX_P2] == 1) k->b[PAR_FX_P2] = 127;   /* 0.5's delay range "hi" */
}

/* an old project image (OLD_PROJECT_SIZE bytes) into a Project */
static void project_from_old(Project *p, const unsigned char *old)
{
    int i;
    const size_t tail = sizeof(Project) - offsetof(Project, pats);
    memcpy(p->name, old, NAME_LEN);
    for (i = 0; i < NUM_KITS; i++) lxr_kitFromOld(&p->kits[i], old + NAME_LEN + (size_t)i * OLD_KIT_SIZE);
    memcpy(&p->pats, old + NAME_LEN + NUM_KITS * OLD_KIT_SIZE, tail);
}

static void mkdirs(const char *path)
{
    char tmp[600];
    char *p;
    snprintf(tmp, sizeof tmp, "%s", path);
    for (p = tmp + 1; *p; p++)
        if (*p == '/') { *p = 0; mkdir(tmp, 0755); *p = '/'; }
    mkdir(tmp, 0755);
}

static void proj_path(Lxr *x, uint8_t slot, char *out, size_t n)
{
    snprintf(out, n, "%s/Projects/proj%02d.lxr", x->dataDir, slot + 1);
}

/* ---- writing on a worker thread ---- */
typedef struct { char path[600]; char dir[600]; unsigned char *buf; size_t len; } WriteJob;

static void *write_thread(void *arg)
{
    WriteJob *j = arg;
    char tmp[620];
    FILE *f;
    mkdirs(j->dir);
    snprintf(tmp, sizeof tmp, "%s.new", j->path);
    f = fopen(tmp, "wb");
    if (f) {
        const size_t w = fwrite(j->buf, 1, j->len, f);
        if (fclose(f) == 0 && w == j->len) rename(tmp, j->path);   /* never leave a half-written project */
        else remove(tmp);
    }
    free(j->buf);
    free(j);
    return NULL;
}

static int write_async(const char *dir, const char *path, unsigned char *buf, size_t len)
{
    pthread_t th;
    pthread_attr_t at;
    WriteJob *j = calloc(1, sizeof *j);
    if (!j) { free(buf); return 0; }
    snprintf(j->path, sizeof j->path, "%s", path);
    snprintf(j->dir, sizeof j->dir, "%s", dir);
    j->buf = buf;
    j->len = len;
    pthread_attr_init(&at);
    pthread_attr_setdetachstate(&at, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&th, &at, write_thread, j) != 0) { pthread_attr_destroy(&at); write_thread(j); return 1; }
    pthread_attr_destroy(&at);
    return 1;
}

int store_saveProject(Lxr *x, uint8_t slot)
{
    char dir[600], path[600];
    unsigned char *buf;
    const size_t len = 8 + sizeof(Project);
    if (!x->dataDir[0]) return 0;
    buf = malloc(len);
    if (!buf) return 0;
    memcpy(buf, PROJ_MAGIC, 8);
    memcpy(buf + 8, &x->p, sizeof(Project));   /* a snapshot: the card write happens later, off the audio thread */
    snprintf(dir, sizeof dir, "%s/Projects", x->dataDir);
    proj_path(x, slot, path, sizeof path);
    return write_async(dir, path, buf, len);
}

/* ---- loading: read on a worker, handed over at the next audio block (engine.c calls store_poll) ---- */
typedef struct { Lxr *x; char path[600]; uint8_t slot; } ReadJob;
static pthread_mutex_t g_loadLock = PTHREAD_MUTEX_INITIALIZER;
typedef struct Loaded { Lxr *x; Project *p; uint8_t slot; struct Loaded *next; } Loaded;
static Loaded *g_loaded;

static void *read_thread(void *arg)
{
    ReadJob *j = arg;
    FILE *f = fopen(j->path, "rb");
    if (f) {
        char magic[8];
        Project *p = malloc(sizeof *p);
        int ok = 0;
        if (p && fread(magic, 1, 8, f) == 8) {
            if (!memcmp(magic, PROJ_MAGIC, 8)) ok = fread(p, 1, sizeof *p, f) == sizeof *p;
            else if (!memcmp(magic, PROJ_MAGIC_V1, 8)) {
                unsigned char *old = malloc(OLD_PROJECT_SIZE);
                if (old && fread(old, 1, OLD_PROJECT_SIZE, f) == OLD_PROJECT_SIZE) { project_from_old(p, old); ok = 1; }
                free(old);
            }
        }
        if (ok) {
            Loaded *l = calloc(1, sizeof *l);
            if (l) {
                l->x = j->x; l->p = p; l->slot = j->slot;
                pthread_mutex_lock(&g_loadLock);
                l->next = g_loaded; g_loaded = l;
                pthread_mutex_unlock(&g_loadLock);
                p = NULL;
            }
        }
        free(p);
        fclose(f);
    }
    free(j);
    return NULL;
}

int store_loadProject(Lxr *x, uint8_t slot)
{
    pthread_t th;
    pthread_attr_t at;
    ReadJob *j;
    struct stat st;
    char path[600];
    proj_path(x, slot, path, sizeof path);
    if (stat(path, &st) != 0) return 0;
    j = calloc(1, sizeof *j);
    if (!j) return 0;
    j->x = x;
    j->slot = slot;
    snprintf(j->path, sizeof j->path, "%s", path);
    pthread_attr_init(&at);
    pthread_attr_setdetachstate(&at, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&th, &at, read_thread, j) != 0) read_thread(j);
    pthread_attr_destroy(&at);
    return 1;
}

/* engine.c, with the instance's engine state in: take a finished load */
void store_poll(Lxr *x)
{
    Loaded **pp, *got = NULL;
    if (!g_loaded) return;
    pthread_mutex_lock(&g_loadLock);
    for (pp = &g_loaded; *pp; pp = &(*pp)->next)
        if ((*pp)->x == x) { got = *pp; *pp = got->next; break; }
    pthread_mutex_unlock(&g_loadLock);
    if (!got) return;
    seq_stop(x);
    x->p = *got->p;
    x->projSlot = got->slot;
    x->kitSlot = 0;
    x->kit = x->p.kits[0];
    x->morphLoaded = 0;
    x->morph = 0;
    seq_loadPattern(x, 0);
    lxr_applyKit(x);
    ui_message(x, "Loading Project", "done", 800);
    ui_refreshSlots(x);
    free(got->p);
    free(got);
}

void store_forget(Lxr *x)
{
    Loaded **pp;
    pthread_mutex_lock(&g_loadLock);
    for (pp = &g_loaded; *pp;) {
        if ((*pp)->x == x) { Loaded *d = *pp; *pp = d->next; free(d->p); free(d); }
        else pp = &(*pp)->next;
    }
    pthread_mutex_unlock(&g_loadLock);
}

int store_projectName(Lxr *x, uint8_t slot, char *name)
{
    char path[600], magic[8];
    FILE *f;
    int ok = 0;
    proj_path(x, slot, path, sizeof path);
    f = fopen(path, "rb");
    if (!f) return 0;
    if (fread(magic, 1, 8, f) == 8 && (!memcmp(magic, PROJ_MAGIC, 8) || !memcmp(magic, PROJ_MAGIC_V1, 8))
        && fread(name, 1, NAME_LEN, f) == NAME_LEN) {
        int i;
        for (i = 0; i < NAME_LEN; i++) if (name[i] < 32 || name[i] > 126) name[i] = ' ';
        name[NAME_LEN] = 0;
        while (name[0] && name[strlen(name) - 1] == ' ') name[strlen(name) - 1] = 0;
        ok = 1;
    }
    fclose(f);
    return ok;
}

/* ---- device-wide settings: the pads (learned layout, panel or notes) ---- */
void store_loadSettings(Lxr *x)
{
    char path[600];
    FILE *f;
    int base, col, row, mode, learned;
    if (!x->dataDir[0]) return;
    snprintf(path, sizeof path, "%s/settings.cfg", x->dataDir);
    f = fopen(path, "r");
    if (!f) return;
    /* v2 settings only: 0.1's file held an assumed layout, never a learned one */
    if (fscanf(f, "pads=2\npad_learned=%d\npad_base=%d\npad_col=%d\npad_row=%d\npad_mode=%d",
               &learned, &base, &col, &row, &mode) == 5 && base >= 0 && base < 128 && col > -128 && col < 128 && col != 0
        && row > -128 && row < 128 && row != 0) {
        x->padLearned = (uint8_t)(learned != 0);
        x->padBase = (uint8_t)base;
        x->padCol = (int8_t)col;
        x->padRow = (int8_t)row;
        x->padMode = (uint8_t)(mode != 0);
    }
    fclose(f);
}

void store_saveSettings(Lxr *x)
{
    char path[600], dir[600];
    unsigned char *buf;
    int n;
    if (!x->dataDir[0]) return;
    buf = malloc(160);
    if (!buf) return;
    n = snprintf((char *)buf, 160, "pads=2\npad_learned=%d\npad_base=%d\npad_col=%d\npad_row=%d\npad_mode=%d\n",
                 x->padLearned, x->padBase, x->padCol, x->padRow, x->padMode);
    snprintf(dir, sizeof dir, "%s", x->dataDir);
    snprintf(path, sizeof path, "%s/settings.cfg", x->dataDir);
    write_async(dir, path, buf, (size_t)n);
}

/* ---------------------------------------------------------------- the MPC state chunk
 * Binary state, run-length packed (PackBits: an empty pattern is mostly repeats), then base64: the plugin wrapper
 * hands chunks over as text. */
typedef struct {
    char magic[4];
    uint8_t version;
    uint8_t kitSlot, patSlot, songSlot, projSlot;
    uint8_t morphLoaded, morph, mutes;
    uint8_t track, section, mode;
    uint8_t fader[NUM_FADERS];
    uint8_t pad[8];
} StateHead;

static size_t packbits(const unsigned char *in, size_t n, unsigned char *out)
{
    size_t i = 0, o = 0;
    while (i < n) {
        size_t run = 1;
        while (i + run < n && run < 128 && in[i + run] == in[i]) run++;
        if (run >= 3) {
            out[o++] = (unsigned char)(257 - run);   /* -(run-1) */
            out[o++] = in[i];
            i += run;
        } else {
            size_t lit = 0;
            while (i + lit < n && lit < 128) {
                if (i + lit + 2 < n && in[i + lit] == in[i + lit + 1] && in[i + lit] == in[i + lit + 2]) break;
                lit++;
            }
            out[o++] = (unsigned char)(lit - 1);
            memcpy(out + o, in + i, lit);
            o += lit;
            i += lit;
        }
    }
    return o;
}

static size_t unpackbits(const unsigned char *in, size_t n, unsigned char *out, size_t cap)
{
    size_t i = 0, o = 0;
    while (i < n && o < cap) {
        const unsigned char h = in[i++];
        if (h < 128) {
            size_t lit = (size_t)h + 1;
            if (i + lit > n || o + lit > cap) return 0;
            memcpy(out + o, in + i, lit);
            i += lit; o += lit;
        } else if (h > 128) {
            size_t run = 257 - h;
            if (i >= n || o + run > cap) return 0;
            memset(out + o, in[i++], run);
            o += run;
        }
    }
    return o;
}

static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* Before PackBits, each byte is XORed with the one a record earlier (kits against the kit before, everything else
 * against 8 bytes before: a step), so the many equal kits and steps become runs of zeros and the chunk stays small. */
#define KITS_AT (sizeof(StateHead) + offsetof(Project, kits))
static void delta_encode(unsigned char *raw, size_t n)
{
    size_t i;
    unsigned char *k = raw + KITS_AT;
    for (i = NUM_KITS * sizeof(Kit); i-- > sizeof(Kit);) k[i] ^= k[i - sizeof(Kit)];
    for (i = n; i-- > 8;) raw[i] ^= raw[i - 8];
}

static void delta_decode(unsigned char *raw, size_t n, size_t kitSize)
{
    size_t i;
    unsigned char *k = raw + KITS_AT;
    for (i = 8; i < n; i++) raw[i] ^= raw[i - 8];
    for (i = kitSize; i < NUM_KITS * kitSize; i++) k[i] ^= k[i - kitSize];
}

char *store_serialize(Lxr *x, int *outLen)
{
    StateHead h;
    const size_t rawLen = sizeof h + sizeof(Project) + 2 * sizeof(Kit) + sizeof(Pattern) + sizeof x->fader;
    unsigned char *raw = malloc(rawLen), *packed = malloc(rawLen + rawLen / 64 + 16);
    char *text;
    size_t plen, i, o = 0;
    int k;
    if (!raw || !packed) { free(raw); free(packed); return NULL; }
    memset(&h, 0, sizeof h);
    memcpy(h.magic, STATE_MAGIC, 4);
    h.version = 3;   /* 2: the voices' default notes are the Force pads' (lxr_padNotes); 3: LXR-02 kits (247 bytes) */
    h.kitSlot = x->kitSlot; h.patSlot = x->patSlot; h.songSlot = x->songSlot; h.projSlot = x->projSlot;
    h.morphLoaded = x->morphLoaded; h.morph = x->morph; h.mutes = x->seq.mutes;
    h.track = x->ui.track; h.section = x->ui.section; h.mode = M_VOICE;
    h.pad[0] = (uint8_t)(x->rnd.amt + 1); h.pad[1] = (uint8_t)(x->rnd.seqAmt + 1);
    for (k = 0; k < NUM_FADERS; k++) h.fader[k] = (uint8_t)(x->fader[k] * 255.f + 0.5f);
    memcpy(raw, &h, sizeof h);
    memcpy(raw + sizeof h, &x->p, sizeof(Project));
    memcpy(raw + sizeof h + sizeof(Project), &x->kit, sizeof(Kit));
    memcpy(raw + sizeof h + sizeof(Project) + sizeof(Kit), &x->morphKit, sizeof(Kit));
    memcpy(raw + sizeof h + sizeof(Project) + 2 * sizeof(Kit), &x->pat, sizeof(Pattern));
    memcpy(raw + sizeof h + sizeof(Project) + 2 * sizeof(Kit) + sizeof(Pattern), x->fader, sizeof x->fader);   /* 3: exact */
    delta_encode(raw, rawLen);
    plen = packbits(raw, rawLen, packed);
    free(raw);
    text = malloc(16 + (plen + 2) / 3 * 4 + 1);
    if (!text) { free(packed); return NULL; }
    o = (size_t)sprintf(text, "LXS1:%lu:", (unsigned long)rawLen);
    for (i = 0; i < plen; i += 3) {
        const unsigned v = (unsigned)packed[i] << 16 | (i + 1 < plen ? (unsigned)packed[i + 1] << 8 : 0) | (i + 2 < plen ? packed[i + 2] : 0);
        text[o++] = b64[(v >> 18) & 63];
        text[o++] = b64[(v >> 12) & 63];
        text[o++] = i + 1 < plen ? b64[(v >> 6) & 63] : '=';
        text[o++] = i + 2 < plen ? b64[v & 63] : '=';
    }
    text[o] = 0;
    free(packed);
    if (outLen) *outLen = (int)o;
    return text;
}

static int b64val(char c)
{
    const char *p = strchr(b64, c);
    return c && p ? (int)(p - b64) : -1;
}

int store_deserialize(Lxr *x, const char *text)
{
    unsigned long rawLen;
    const char *p;
    size_t n, plen = 0, i;
    unsigned char *packed, *raw;
    StateHead h;
    int k, old;
    const size_t newLen = sizeof h + sizeof(Project) + 2 * sizeof(Kit) + sizeof(Pattern) + sizeof x->fader;
    const size_t oldLen = sizeof h + OLD_PROJECT_SIZE + 2 * OLD_KIT_SIZE + sizeof(Pattern);
    if (strncmp(text, "LXS1:", 5) != 0) return 0;
    rawLen = strtoul(text + 5, NULL, 10);
    p = strchr(text + 5, ':');
    if (!p || (rawLen != newLen && rawLen != oldLen)) return 0;
    old = rawLen == oldLen;   /* saved by LXRXXL 0.5.x */
    p++;
    n = strlen(p);
    packed = malloc(n / 4 * 3 + 3);
    raw = malloc(rawLen);
    if (!packed || !raw) { free(packed); free(raw); return 0; }
    for (i = 0; i + 3 < n + 0 && i + 4 <= n; i += 4) {
        const int a = b64val(p[i]), b = b64val(p[i + 1]), c = p[i + 2] == '=' ? 0 : b64val(p[i + 2]), d = p[i + 3] == '=' ? 0 : b64val(p[i + 3]);
        unsigned v;
        if (a < 0 || b < 0 || c < 0 || d < 0) { free(packed); free(raw); return 0; }
        v = (unsigned)a << 18 | (unsigned)b << 12 | (unsigned)c << 6 | (unsigned)d;
        packed[plen++] = (unsigned char)(v >> 16);
        if (p[i + 2] != '=') packed[plen++] = (unsigned char)(v >> 8);
        if (p[i + 3] != '=') packed[plen++] = (unsigned char)v;
    }
    if (unpackbits(packed, plen, raw, rawLen) != rawLen) { free(packed); free(raw); return 0; }
    free(packed);
    delta_decode(raw, rawLen, old ? OLD_KIT_SIZE : sizeof(Kit));
    memcpy(&h, raw, sizeof h);
    if (memcmp(h.magic, STATE_MAGIC, 4) != 0) { free(raw); return 0; }
    seq_stop(x);
    if (old) {
        const unsigned char *o = raw + sizeof h;
        project_from_old(&x->p, o);
        lxr_kitFromOld(&x->kit, o + OLD_PROJECT_SIZE);
        lxr_kitFromOld(&x->morphKit, o + OLD_PROJECT_SIZE + OLD_KIT_SIZE);
        memcpy(&x->pat, o + OLD_PROJECT_SIZE + 2 * OLD_KIT_SIZE, sizeof(Pattern));
    } else {
        memcpy(&x->p, raw + sizeof h, sizeof(Project));
        memcpy(&x->kit, raw + sizeof h + sizeof(Project), sizeof(Kit));
        memcpy(&x->morphKit, raw + sizeof h + sizeof(Project) + sizeof(Kit), sizeof(Kit));
        memcpy(&x->pat, raw + sizeof h + sizeof(Project) + 2 * sizeof(Kit), sizeof(Pattern));
    }
    for (k = 0; k < NUM_FADERS; k++) x->fader[k] = h.fader[k] / 255.f;
    if (!old) {   /* 0.6: the faders' exact positions after the pattern (the header keeps the 8-bit ones too) */
        float f[NUM_FADERS];
        memcpy(f, raw + sizeof h + sizeof(Project) + 2 * sizeof(Kit) + sizeof(Pattern), sizeof f);
        for (k = 0; k < NUM_FADERS; k++) if (f[k] >= 0.f && f[k] <= 1.f) x->fader[k] = f[k];
    }
    if (h.version < 2) {   /* 0.1-0.3 put the voices on 36-42: those untouched move to the pads' notes */
        uint8_t t;
        for (t = 0; t < NUM_TRACKS; t++)
            if (x->p.g.trackNote[t] == 36 + t) x->p.g.trackNote[t] = lxr_padNotes[t];
    }
    free(raw);
    x->kitSlot = h.kitSlot < NUM_KITS ? h.kitSlot : 0;
    x->patSlot = h.patSlot < NUM_PATTERNS ? h.patSlot : 0;
    x->songSlot = h.songSlot < NUM_SONGS ? h.songSlot : 0;
    x->projSlot = h.projSlot < NUM_PROJECTS ? h.projSlot : 0;
    x->morphLoaded = h.morphLoaded;
    x->morph = h.morph;
    x->seq.mutes = h.mutes;
    x->ui.track = h.track < NUM_TRACKS ? h.track : 0;
    x->ui.section = h.section < 8 ? h.section : 0;
    if (h.pad[0] >= 1 && h.pad[0] <= 11) x->rnd.amt = (uint8_t)(h.pad[0] - 1);
    if (h.pad[1] >= 1 && h.pad[1] <= 11) x->rnd.seqAmt = (uint8_t)(h.pad[1] - 1);
    lxr_applyKit(x);
    ui_refreshSlots(x);
    return 1;
}

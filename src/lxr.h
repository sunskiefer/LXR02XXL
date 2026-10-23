/* LXRXXL -- one LXR-02 instance: the project in RAM, the sequencer, the front panel and the sound engine.
 *
 * Memory model as on the machine (owner's manual ch. 4): a PROJECT holds 64 kits, 64 patterns and 64 songs plus the
 * global settings. Everything is edited in RAM; the working kit and the playing pattern are copies that a save writes
 * back to their slot, and only "save project" writes the project to the card (here: the plugin's projects folder).
 * MPC's own project save keeps the whole RAM state of the instance (state chunk), so nothing is lost with the song. */
#pragma once
#include <stdint.h>
#include "lxr/lxr_dsp.h"

#define NUM_TRACKS   7
#define NUM_STEPS    64
#define NUM_PATTERNS 64
#define NUM_KITS     64
#define NUM_SONGS    64
#define SONG_LEN     64
#define NUM_PROJECTS 64
#define NAME_LEN     8

#define SONG_END  0xFE
#define SONG_LOOP 0xFF

typedef struct {
    uint8_t on;          /* 1 = active */
    uint8_t vel;         /* 0..127 */
    uint8_t note;        /* MIDI note, 63 = the voice's own pitch */
    uint8_t prob;        /* 0..100 % */
    uint8_t p1, v1;      /* automation lane 1: parameter + 1 (0 = none), value */
    uint8_t p2, v2;      /* automation lane 2 */
} Step;

typedef struct {
    Step s[NUM_TRACKS][NUM_STEPS];
    uint8_t len[NUM_TRACKS];   /* 1..64 */
    uint8_t kit;               /* the kit saved with the pattern (kit change mode "auto") */
    uint8_t pad[2];
} Pattern;

typedef struct {
    char name[NAME_LEN];
    uint8_t b[LXR_KIT_BYTES];
} Kit;

typedef struct {
    char name[NAME_LEN];
    uint8_t e[SONG_LEN];       /* pattern 0..63, SONG_END, SONG_LOOP */
} Song;

typedef struct {               /* the global settings menu (7.6) + what the manual keeps per project */
    uint8_t bpm;               /* 0 = Ext: follow MPC's tempo and transport */
    uint8_t midiCh;            /* global MIDI channel 0..15 */
    uint8_t mtx, mrx;          /* MIDI filters: bit 0 N, 1 R, 2 C, 3 P (15 = all) */
    uint8_t mrt;               /* MIDI routing (no physical ports on MPC: kept for the menu) */
    uint8_t ssv;               /* screensaver */
    uint8_t flw;               /* bar follow */
    uint8_t kcm;               /* kit change mode: 0 off, 1 auto */
    uint8_t cki, cko;          /* clock PPQ (no jacks on MPC: kept for the menu) */
    uint8_t trackCh[NUM_TRACKS];   /* voice MIDI channel 0..15 */
    uint8_t trackNote[NUM_TRACKS]; /* voice MIDI note, 0 = any (tonal) */
    uint8_t shuffle;           /* 0..127 */
    uint8_t roll;              /* roll rate index */
    uint8_t recTrack;          /* automation track 0/1 */
    uint8_t quant;             /* record quantisation */
    uint8_t pad[8];
} Globals;

typedef struct {
    char name[NAME_LEN];
    Kit kits[NUM_KITS];
    Pattern pats[NUM_PATTERNS];
    Song songs[NUM_SONGS];
    Globals g;
} Project;

/* the factory content (src/factory_data.c, made by tools/import_lxr02.py from an LXR-02 project) */
typedef struct {
    uint8_t pat, kit, len[NUM_TRACKS];
    Step s[NUM_TRACKS][NUM_STEPS];
} LxrFactoryPattern;
extern const Kit lxr_factoryKits[NUM_KITS];
extern const LxrFactoryPattern lxr_factoryPatterns[];
extern const int lxr_numFactoryPatterns;
extern const uint8_t lxr_factoryKits05[16][253];   /* factory_05.c: the 0.5.x factory kits, old layout */

/* ---------------------------------------------------------------- the front panel */
enum {   /* buttons, in the order of their plugin parameters */
    B_SHIFT, B_PLAY, B_REC, B_COPY, B_VOICE, B_PERF, B_LOAD, B_SAVE,
    B_SEL1, B_SEL2, B_SEL3, B_SEL4, B_SEL5, B_SEL6, B_SEL7, B_SEL8,
    B_BARL, B_BARR,
    B_V1, B_V2, B_V3, B_V4, B_V5, B_V6, B_V7,
    B_S1, B_S2, B_S3, B_S4, B_S5, B_S6, B_S7, B_S8, B_S9, B_S10, B_S11, B_S12, B_S13, B_S14, B_S15, B_S16,
    B_ENC,
    NUM_BUTTONS
};
#define NUM_SLOTS 8
#define NUM_FADERS 7

enum Mode {
    M_VOICE, M_STEP, M_PERF, M_PATGEN, M_LOAD, M_SAVE, M_CONFIG, M_PATTERN, M_SONG, M_FX, M_LASTSTEP, M_REC, M_CLEAR,
    M_PADLEARN
};

typedef struct {
    uint8_t kind, id;   /* see ui.c */
} Slot;

typedef struct {                /* the sequencer's position (seq.c) */
    uint32_t n;                 /* the absolute 16th that fires next */
    uint32_t patStart;          /* the absolute 16th where the playing pattern started */
    double pos;                 /* the current position in 96 ppq ticks */
    double rollMark[7];
    uint8_t skipNext[7];
} SeqRun;

typedef struct Lxr {
    Project p;
    Kit kit;                   /* the working kit (what plays) */
    Kit morphKit;              /* LOAD + MORPH */
    uint8_t morphLoaded, morph;   /* morph 0..255 */
    Pattern pat;               /* the working pattern: what plays and what the buttons edit */
    uint8_t kitSlot;           /* the kit last loaded/saved (0..63) */
    uint8_t patSlot;           /* the pattern in pat */
    uint8_t songSlot;
    uint8_t projSlot;
    uint8_t dirtyKit;

    /* sequencer */
    struct {
        uint8_t running, songMode, recording, erasing;
        uint8_t freeRun;           /* started by the LXR's own PLAY while following MPC's tempo: MPC's position not used */
        uint8_t nextPat;           /* the pattern queued by the pattern menu (0xff none) */
        uint8_t songPos;
        double tick;               /* position in 96 ppq ticks since start */
        uint32_t sixteenth;        /* 16ths played since start */
        uint8_t pos[NUM_TRACKS];   /* the step each track plays next / last played */
        uint8_t lastPos[NUM_TRACKS];
        uint16_t masterPos;        /* 16ths into the pattern */
        uint8_t mutes;             /* bit per track */
        uint8_t rolls;             /* bit per track: manual roll on */
        float bpm;                 /* tempo in use (internal or MPC's) */
        /* MPC transport */
        uint8_t hostPlaying;
        double hostPpq, hostTempo;
        int hostValid;
        double lastPpq;
        double nextStepPpq;
        uint8_t pendingStart;
        uint32_t frames;           /* sample clock */
        double framePos;           /* internal clock: frames to the next 1/96 tick */
        uint8_t beatLed;
    } seq;
    SeqRun run;

    /* front panel */
    struct {
        uint8_t mode, prevMode;
        uint8_t shift, copy;       /* latched (touch) or held (pads) */
        uint8_t shiftHeld, copyHeld;   /* held from a pad */
        uint8_t held[NUM_BUTTONS];     /* pads: buttons physically held */
        uint8_t track;             /* active track 0..6 */
        uint8_t section;           /* 0..7: OSC AEG MOD FM CLICK FILTER LFO MIX */
        uint8_t sel;               /* the selected slot 0..7 (capitals on the LCD) */
        uint8_t bar;               /* viewed bar 0..3 */
        uint8_t step;              /* the selected step 0..63 (step mode) */
        uint8_t group;             /* pattern group 0..3 (pattern pages) */
        uint8_t lsType;            /* load/save: 0 kit, 1 morph, 2 song, 3 project, 4 pattern */
        uint8_t lsSlot;
        uint8_t lsArm;             /* LOAD/SAVE pressed, waiting for the data type key */
        uint8_t lsState;           /* save: 0 slot, 1 "edit name?" , 2 name edit */
        uint8_t lsYes;             /* save: edit name y/n */
        uint8_t nameCursor;
        char name[NAME_LEN];
        uint8_t clearSel;          /* clear menu: what */
        uint8_t copySrc;           /* copy: source track (0xff none) */
        uint8_t lastStepAll;
        uint8_t songRow, songCol;
        uint8_t genLen[NUM_TRACKS], genSteps[NUM_TRACKS], genRot[NUM_TRACKS];
        int8_t heldStep;           /* a step key held on a pad (parameter lock), -1 none */
        uint8_t heldStepUsed;
        uint8_t learnStage;
        uint8_t learnNotes[3];
        uint32_t msgUntil;         /* frames: a temporary LCD message */
        char msg[2][32];
        uint32_t pulse[NUM_TRACKS];   /* voice LED pulse until (frames) */
        uint32_t pressedAt[NUM_BUTTONS];
        uint8_t potHalf;           /* which half of the slots the 4 pots edit */
        float dataKnob;            /* where the DATA knob's picture stands: it turns only when DATA turns (+1: set) */
    } ui;

    Slot slots[NUM_SLOTS];
    float fader[NUM_FADERS];
    uint32_t displayRev;
    uint32_t clock;            /* frames rendered since the instance started (blinks, messages) */
    float outL[LXR_BLOCK], outR[LXR_BLOCK];  /* the engine block being played out */
    int outPos;
    uint8_t ledShown[NUM_BUTTONS];
    uint32_t pressFlip[NUM_BUTTONS];   /* host button resync (see engine.c) */

    /* pads */
    int nrpn;                  /* the NRPN number being set over MIDI (-1: none) */
    uint8_t padMode;           /* 0 panel, 1 notes */
    uint8_t padLearned;        /* the pads' notes were learned (pad learn): until then the pads play the voices */
    uint8_t padBase;           /* learned: the bottom-left pad's note */
    int8_t padCol, padRow;     /* learned: note step to the next pad right / to the pad above */

    struct {                   /* the mod's randomizers (random.c) */
        uint64_t state;        /* the instance's random generator */
        uint8_t prev[LXR_KIT_BYTES];   /* each parameter's value before the last press: never landed on again */
        uint8_t amt, seqAmt;   /* the knobs: 0..10 = 0..100 % */
        uint32_t flash, seqFlash, resetFlash;   /* the keys light until (frames) */
    } rnd;
    uint8_t pendingTrig[NUM_TRACKS];   /* velocity+1 queued by a trigger from the UI thread */
    uint8_t pendingNote[NUM_TRACKS];
    char dataDir[512];
    struct LxrState *state;            /* the engine's globals for this instance (lxr_state.c) */
} Lxr;

/* lxr.c / seq.c / ui.c */
void lxr_init(Lxr *x, const char *dataDir);
void lxr_applyKit(Lxr *x);                         /* the working kit (and morph) into the engine */
void lxr_setKitParam(Lxr *x, uint16_t par, uint8_t value, int record);   /* edit + engine (+ automation recording) */
void lxr_setExt(Lxr *x, uint8_t ext, uint8_t value);
void lxr_render(Lxr *x, float *L, float *R, int n);     /* any n; the engine runs in 64-frame blocks */
void lxr_trigger(Lxr *x, uint8_t track, uint8_t vel, uint8_t note, int fromUser);
void lxr_initKit(Kit *k);
void lxr_initPattern(Pattern *p);
void lxr_initProject(Project *p);
void lxr_factoryProject(Project *p);               /* the imported LXR-02 project (factory_data.c) */
void lxr_loadFactory(Lxr *x);                      /* the RESET key: back to the factory project */
extern const uint8_t lxr_padNotes[NUM_TRACKS];     /* the voices' default notes: the Force's pads 1-7 */

void seq_start(Lxr *x, int song);
void seq_stop(Lxr *x);
void seq_block(Lxr *x, int frames);                 /* advance the clock by an engine block, firing what falls in it */
void seq_hostTime(Lxr *x, int playing, double ppq, double tempo);
void seq_loadPattern(Lxr *x, uint8_t pat);         /* now: working pattern = stored pattern */
void seq_queuePattern(Lxr *x, uint8_t pat);
void seq_recordNote(Lxr *x, uint8_t track, uint8_t vel, uint8_t note);
void seq_recordParam(Lxr *x, uint16_t par, uint8_t value);
uint8_t seq_patternLength(const Pattern *p);
void seq_euclid(Lxr *x, uint8_t track);
void seq_clearTrackAt(Lxr *x, uint8_t track);       /* live erase at the playhead */

void ui_init(Lxr *x);
void ui_press(Lxr *x, int b, int fromPad);
void ui_release(Lxr *x, int b, int fromPad);
void ui_data(Lxr *x, int steps);                    /* the DATA encoder */
void ui_pot(Lxr *x, int pot, int steps);
float ui_potGet(Lxr *x, int pot);                   /* the pot's position: the value it edits, 0..1 */
void ui_potSet(Lxr *x, int pot, float v);
float ui_dataGet(Lxr *x);                           /* the DATA encoder's position in what it moves through */
void ui_dataSet(Lxr *x, float v);            /* the 4 pots */
void ui_slotSet(Lxr *x, int slot, float norm);      /* a slider on the screen */
float ui_slotGet(Lxr *x, int slot);
const char *ui_slotName(Lxr *x, int slot, char *buf);
const char *ui_slotDisplay(Lxr *x, int slot, char *buf);
int ui_slotUsed(Lxr *x, int slot);
int ui_led(Lxr *x, int b);
void ui_lcd(Lxr *x, char *l1, char *l2);
void ui_refreshSlots(Lxr *x);
void ui_message(Lxr *x, const char *l1, const char *l2, int ms);
void ui_padNote(Lxr *x, uint8_t note, uint8_t vel);   /* a pad: on (vel > 0) or off */
int ui_padButton(Lxr *x, uint8_t note);              /* the button a pad note is, -1 none */
const char *ui_trackName(int t);

/* random.c (the mod) */
void lxr_randomizeVoices(Lxr *x, int pct, int voice);   /* the green key (voice -1: all six) */
void seq_randomize(Lxr *x, int pct, int track);          /* the yellow key (track -1: all seven) */

/* store.c */
void lxr_kitFromOld(Kit *k, const unsigned char *old);   /* a kit saved by LXRXXL 0.5.x (8 + 245 bytes) */
int store_saveProject(Lxr *x, uint8_t slot);
int store_loadProject(Lxr *x, uint8_t slot);
int store_projectName(Lxr *x, uint8_t slot, char *name);   /* 1 if it exists */
char *store_serialize(Lxr *x, int *len);                   /* the state chunk (malloc'd text) */
int store_deserialize(Lxr *x, const char *text);
void store_poll(Lxr *x);                                  /* a project load finished: take it (audio side) */
void store_forget(Lxr *x);
void store_loadSettings(Lxr *x);                           /* device-wide: pad learn */
void store_saveSettings(Lxr *x);

/* meta.c */
uint8_t meta_soundMax(uint16_t par);
void meta_soundText(uint16_t par, uint8_t value, char *buf);    /* the value as the LXR shows it */
const char *meta_textShort(uint8_t textId);      /* NamesEnum (menu.h): the LCD's 3-letter name */
const char *meta_textLong(uint8_t textId);       /* the detail page's long name */
const char *meta_textCat(uint8_t textId);        /* its category ("Filter", "LFO"...) */
void meta_noteName(uint8_t note, char *buf);
uint8_t meta_numModTargets(void);
void meta_modTargetRange(uint8_t voice0, uint8_t *start, uint8_t *end);
void meta_modTargetText(uint8_t idx, char *buf, int withVoice);
uint16_t meta_modTargetParam(uint8_t idx);
uint8_t meta_paramToModTarget(uint16_t par);
const char *meta_targetLong(uint8_t idx);
uint32_t lxr_clock(Lxr *x);

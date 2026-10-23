# LXRXXL user manual

LXRXXL 0.6.0, for the Akai Force and MPC (MPC OS 3.x). The LXR-02's open-source sound engine (the Erica Synths LXR
firmware's, GPL-3.0) with its own sequencer and front panel, inspired by the Erica Synths LXR-02. Artwork by Suns Kiefer.

Contents: [1 The page](#1-the-page) · [2 Playing and the transport](#2-playing-and-the-transport) ·
[3 The keys](#3-the-keys) · [4 Sound editing](#4-sound-editing-voice-mode) · [5 The sequencer](#5-the-sequencer) ·
[6 Patterns and songs](#6-patterns-and-songs) · [7 Performance](#7-performance) · [8 FX](#8-the-fx-unit) ·
[9 Load, save and clear](#9-load-save-and-clear) · [10 The mod](#10-the-mod-reset-and-the-randomizers) ·
[11 Global settings](#11-global-settings-config) · [12 Force pads, Q-Links and MIDI](#12-force-pads-q-links-and-midi) ·
[13 Files](#13-files-and-projects) · [14 Quick reference](#14-quick-reference)

---

## 1 The page

LXRXXL is one page: the panel on the right, the PARAMETERS column on the left.

| Part | What it does |
|---|---|
| **Display** | The LXR's two lines. Line 1: what you edit (voice and page, e.g. `DRUM1 OSC`) and the pattern (`P01`). Line 2: the selected value's full name and value (`Coarse 44`). Messages ("loaded", "copy bar") show here for a moment. |
| **PARAMETERS column** | Every value of the page on the display, one row each: its short name, its value, and a bar to drag. The selected value's name is in capitals. A page with fewer values shows fewer rows. |
| **4 pots** | Values 1-4 of the page, or 5-8 once one of those is selected (see 4.1). Each pot stands on its value (full left = minimum, full right = maximum) and turns when the value changes; the value is printed in its white centre. |
| **DATA encoder** | Moves the selection through the page's values (or through the slots when loading/saving, the rows of a song, the clear choices). Its picture turns with the selection from the first value to the last. |
| **DATA push** | A tap on the **DATA** legend under the encoder is the encoder's push. It blinks red when something waits for a confirm (load, save, clear, pad learn). |
| **7 faders** | DRUM1, DRUM2, DRUM3, SNARE, CLP/CYM, HIHATS levels and VOLUME (the master), with the LXR-02's fader curve. |
| **Keys** | Lit red when their LED is on. On the screen a tap is a press; SHIFT and COPY/CLR latch (tap on, tap off). |
| **Step keys 1-16** | Grey = step off, red = step on. The function printed under each one is its SHIFT function or its LOAD/SAVE data type. |
| **RESET, randomizers** | The mod's keys and knobs along the top: see [10](#10-the-mod-reset-and-the-randomizers). |

**SHIFT on the screen:** tap SHIFT (its LED lights), then the key. SHIFT turns off after a SHIFT function. On the
Force's pads (after pad learn, [12](#12-force-pads-q-links-and-midi)) SHIFT is held as on the machine.

---

## 2 Playing and the transport

**Tempo** is set in CONFIG > `bpm` ([11](#11-global-settings-config)):

- **MPC** (the default): the LXR follows MPC. Press play on MPC and the LXR's pattern plays on MPC's grid; stop MPC and
  it stops. While MPC plays, the LXR's PLAY only tells you so ("Tempo: MPC").
- With MPC stopped, **PLAY** runs the LXR on its own at MPC's tempo; PLAY again stops it. The PLAY LED is lit while it
  runs.
- **A number (1-255)**: the LXR's own clock at that BPM. PLAY starts and stops it.

**SHIFT + PLAY** plays the selected song ([6.2](#62-songs)) from its first row. With MPC playing and the tempo on MPC,
SHIFT + PLAY sets song mode for MPC's next play.

**Voices** sound from the Force's pads, MIDI notes, and MPC's own sequencer ([12](#12-force-pads-q-links-and-midi)),
as well as from the LXR's pattern.

---

## 3 The keys

| Key | Press | With SHIFT |
|---|---|---|
| **SHIFT** | arms the SHIFT functions (latches on the screen) | |
| **PLAY** | start / stop the LXR ([2](#2-playing-and-the-transport)) | play the song |
| **REC** | recording on / off ([5.5](#55-recording)) | Record settings (automation track, quantise) |
| **COPY/CLR** | copy: a track or a bar ([5.6](#56-copy)) | the clear menu ([9.4](#94-clear)); while recording: erase at the playhead |
| **VOICE** | voice mode (sound pages); pressed again: step mode | |
| **PERF** | performance mode ([7](#7-performance)) | the euclidean pattern generator ([5.7](#57-euclidean-pattern-generator)) |
| **LOAD / SAVE** | then a data key ([9](#9-load-save-and-clear)) | |
| **OSC AEG MOD FM CLICK FILTER LFO MIX** | the sound pages of the selected voice ([4](#4-sound-editing-voice-mode)); pressed again: the page's values 5-8 on the pots | |
| **BAR ◀ / ▶** | the bar shown on the step keys (1-4); in the pattern and song menus: the group of 16 patterns. Their two LEDs show it: none = 1, right = 2, left = 3, both = 4 | |
| **DRUM1 … OP HH** (voice keys) | select the voice / track | mute / unmute it |
| **Step keys 1-16** | steps of the selected track on the shown bar | the function printed under them (below) |

**SHIFT + step key:**

| Step | Legend | Function |
|---|---|---|
| 10 | SONG | the song editor ([6.2](#62-songs)) |
| 12 | CONFIG | global settings ([11](#11-global-settings-config)) |
| 13 | RELOAD | reload the kit and the pattern as last loaded or saved (drops unsaved edits) |
| 14 | LAST STEP | the track length page ([5.3](#53-track-length-and-last-step)) |
| 15 | FX | the FX unit ([8](#8-the-fx-unit)) |
| 16 | PATTERN | pattern select ([6.1](#61-selecting-patterns)) |

**LOAD / SAVE data keys:** step 8 KIT, 9 MORPH, 10 SONG, 11 PROJECT, 16 PATTERN ([9](#9-load-save-and-clear)).

---

## 4 Sound editing (voice mode)

Six voices, seven tracks: DRUM1, DRUM2, DRUM3, SNARE, CLP/CYM, and the hi-hat voice played by two tracks (CL HH and OP
HH, with separate closed and open decays).

### 4.1 Editing a value

1. Tap **VOICE** (lit), select the voice with its voice key.
2. Tap a page key: **OSC, AEG, MOD, FM, CLICK, FILTER, LFO, MIX**. The display shows `VOICE PAGE`, the column shows the
   page's values.
3. Change a value with: its **row** in the column (drag the bar), a **pot** (pots 1-4 = values 1-4), **DATA** to
   select and a pot or the row to change it, or the Force's data wheel / Q-Links.
4. Pages with more than four values: tap the page key again and the pots edit values 5-8 (or select a value 5-8).

Values on a page, per voice:

| Page | DRUM1-3 | SNARE | CLP/CYM | HIHAT |
|---|---|---|---|---|
| **OSC** | coarse, fine, waveform (Sin Tri Saw Rec Noi PWM), pulse width | coarse, fine, noise frequency, osc/noise mix, waveform, pulse width | coarse, fine, waveform, pulse width | coarse, fine, waveform, pulse width |
| **AEG** (amp envelope) | attack, decay, slope | attack, decay, repeat, slope | attack, decay, repeat (clap), slope | attack, closed decay, open decay, slope |
| **MOD** | pitch envelope decay, slope, amount; velocity destination, velocity amount, velocity to volume | (as DRUM1-3) | velocity destination, amount, velocity to volume | velocity destination, amount, velocity to volume |
| **FM** | FM amount, frequency, modulator waveform, mix | (none: the page is empty) | modulator 1 and 2 frequency and gain, waveforms 2 and 3 | modulator 1 and 2 frequency and gain, waveforms 2 and 3 |
| **CLICK** | transient waveform, volume, frequency | same | same | same |
| **FILTER** | cutoff, resonance, type (LP HP BP UBP Nch Pek LP2), drive | same | same | same |
| **LFO** | rate, sync, amount, waveform, retrigger, offset, destination voice, destination parameter | same | same | same |
| **MIX** | volume, pan, sample rate (decimation), drive, audio out, track length, MIDI channel, MIDI note | same | same | same |

The LFO of each voice can modulate any voice: choose the voice (`voi`), then the parameter (`dst`). The velocity
destination (`dst` on MOD) is a parameter of the same voice. `off` (the first position) means no destination.

### 4.2 Per-step parameters (parameter locks)

Hold a step (on a Force pad) and turn a value: the step stores that value for itself (an automation value). A step keeps
two; a third replaces the oldest. On the screen, use step mode ([5.4](#54-step-mode)) to set them.

### 4.3 Morph

PERF and the pattern generator have `mrp` (Morph, 0-255): it blends the kit with the morph kit loaded with LOAD > MORPH
([9](#9-load-save-and-clear)). MIDI CC 1 (mod wheel) morphs too. SAVE > MORPH saves the blended result as a kit.

---

## 5 The sequencer

64 patterns of 7 tracks × 64 steps (4 bars of 16). Each track has its own length; each step has velocity, note,
probability and two automation values.

### 5.1 Entering steps

In voice mode, select the track (voice key) and the bar (BAR ◀ ▶), then tap step keys: red = on, grey = off. While it
plays, a running light moves along the steps.

### 5.2 Muting

SHIFT + voice key mutes or unmutes that track. In PERF mode the voice keys mute without SHIFT ([7](#7-performance)).

### 5.3 Track length and last step

- **MIX > `len`**: the selected track's length, 1-64.
- **SHIFT + LAST STEP**: the display shows `Last Step` and `Trk: 1 16`. Choose the bar with BAR, then tap the step
  that is the last one. Tap SHIFT + LAST STEP again (double press) to set **all** tracks at once (`Trk: All`).
- **SHIFT + BAR ◀ / ▶** in voice mode: the track shrinks / grows by a bar.

### 5.4 Step mode

Tap **VOICE** a second time (its LED blinks): step mode. Tap a step key to select the step; the display shows
`DRUM1 Step 5`. The column has its values:

| Value | What |
|---|---|
| `vel` | velocity 0-127 |
| `nte` | note (pitch of the voice for this step) |
| `prb` | probability 0-100 %: the chance the step plays each time |
| `p1d` / `p1v` | automation 1: the parameter, its value on this step |
| `p2d` / `p2v` | automation 2: the parameter, its value on this step |

Tap VOICE again to go back to voice mode.

### 5.5 Recording

**REC** (lit) records while the sequencer runs:

- hits from the Force's pads and MIDI notes land on the nearest 16th of their track, with their velocity;
- turning a sound value records it as automation on the step playing, on automation track 1 or 2.

**SHIFT + REC** opens the record settings: `trk` the automation track (1 or 2) and `qnt` the quantise setting.
**SHIFT + COPY/CLR while recording** erases the selected track's steps under the playhead (held on a pad, toggled on
the screen).

### 5.6 Copy

- **A track:** COPY/CLR (blinks), tap the source voice key (it blinks), tap the destination voice key. Steps and length
  are copied.
- **A bar:** COPY/CLR, then BAR ▶: the shown bar is copied into the next one, and the track grows to include it.

### 5.7 Euclidean pattern generator

**SHIFT + PERF** opens `Euklid DRUM1`. For the selected track: `len` (length), `stp` (number of hits), `rot` (rotation),
and `mrp` (morph). Every change writes the track anew: the hits spread as evenly as possible over the length.

### 5.8 Shuffle

PERF > `shu` (0-127): every second 16th plays late, up to two thirds of a 16th.

---

## 6 Patterns and songs

### 6.1 Selecting patterns

**SHIFT + PATTERN** (or LOAD + PATTERN): the display shows `Select Patterns`, `Current: 1`. BAR ◀ ▶ choose the group
(patterns 1-16, 17-32, 33-48, 49-64), a step key picks the pattern. While playing, the new pattern starts at the end
of the current one (`Current: 1 > 3`, its key blinks). With kit change mode on (CONFIG > `kcm` `Aut`), a pattern brings
its kit. SAVE + PATTERN, then a step key, saves the pattern there.

### 6.2 Songs

**SHIFT + SONG** opens the song editor: a list of rows, each a pattern (`p01`-`p64`), `END` or `LOOP`.

- **DATA** moves through the rows; the **DATA push** switches between the row and its pattern, and DATA then changes
  the pattern.
- **In the song editor, a step key** puts that pattern (of the group chosen with BAR) on the row.
- **COPY/CLR** inserts a copy of the row below; **SHIFT + COPY/CLR** deletes the row.
- **SHIFT + PLAY** plays the song.

LOAD / SAVE > SONG loads and saves songs (64 in a project).

---

## 7 Performance

**PERF**: the display shows `Performance`. Values: `bpm` (tempo), `rol` (roll rate: One, 1/1 … 1/8, 12, 16, 24, 32, 48,
64, 128), `mrp` (morph), `sr` (sample rate of all voices), `shu` (shuffle).

- **Step keys 1-7: manual rolls** for DRUM1 … OP HH, repeating at the roll rate. On a pad, held; on the screen, a tap
  turns the roll on, another tap off. With the rate on `One`, a key plays a single hit.
- **Voice keys:** mute / unmute without SHIFT. **SHIFT + voice key** unmutes every track up to that one.

---

## 8 The FX unit

**SHIFT + FX**: one effect at a time on the FX bus (the voices whose audio out on their MIX page is set to FX), saved with the
kit.

| Value | What |
|---|---|
| `typ` | off, drv (drive), rng (ring modulator), cmp (compressor), del (delay), rev (reverb) |
| `out` | where the FX goes: St1, St2, L1, R1, L2, R2 (in the plugin everything ends at MPC's track output) |
| `d/w` | dry / wet, 0-100 |

Per type (the LXR-02 firmware's own effects):

- **Drive:** `typ` tub / fld / clp, `drv` overdrive, `col` colour, `ton` tone, `vol` volume.
- **Ring modulator:** `wav` Sin Tri Saw Rec Noi PWM, `frq` frequency.
- **Compressor:** `rat` 1:1 2:1 3:1 4:1 6:1 8:1, `atk` attack, `dec` decay, `tre` threshold, `gai` gain.
- **Delay:** `typ` Del / Pp (ping-pong), `tim` time, `rng` range (0-127: how far the time reaches), `ton` tone,
  `fbk` feedback.
- **Reverb:** `typ` Plt (plate), `pre` pre-delay, `dec` decay, `ton` tone, `ref` reflection. The firmware's plate
  reverb, which is in its code but switched off on the hardware. While it runs, the snare, clap/cymbal and hi-hat use
  the firmware's lighter filter, as its code does.

LFOs and velocity can modulate each voice's pulse width (after its waveform in the target list) and the FX unit's four
values (at the end of each voice's list), as on the LXR-02.

---

## 9 Load, save and clear

A project holds 64 kits, 64 patterns and 64 songs. Kits, patterns and songs are loaded and saved inside the project in
memory, which MPC's project keeps. **PROJECT** loads and saves the whole project as a file ([13](#13-files-and-projects)).

### 9.1 Loading

1. Tap **LOAD**: the display shows `Load:` and `Kit Mrp Sng Prj Pat`; the data keys light.
2. Tap the data key: **KIT** (step 8), **MORPH** (9), **SONG** (10), **PROJECT** (11) or **PATTERN** (16).
3. Turn **DATA** (the touchscreen encoder, the Force's data wheel or Q-Link 8) to the slot: `[2] Reservoi`.
4. Confirm: tap the blinking **DATA** legend, or tap the same data key again. The display says `loaded`.

**Example, changing kit:** LOAD, KIT, turn DATA to the kit, tap DATA.
Pressing the Force's data wheel in does not reach plugins, so it can't confirm. LOAD again cancels. A pattern loads on
its step key ([6.1](#61-selecting-patterns)).

### 9.2 Saving

1. Tap **SAVE**, then the data key, turn DATA to the slot.
2. Confirm (DATA push or the data key again). The display asks `Edit Name? [y/N]`: DATA to choose, push to confirm.
3. Naming: **pot 1** moves the cursor, **pot 2** switches case, **pot 3** picks the letter (DATA moves the cursor too).
   Move the cursor to `ok` and push to save. The display says `success!`.

### 9.3 Morph kit

LOAD > MORPH loads a kit as the morph target; `mrp` blends towards it. SAVE > MORPH saves the blend as a kit.

### 9.4 Clear

**SHIFT + COPY/CLR**: the display asks `clear [kit]?`. Turn DATA to choose: `kit` (the sound to the LXR-02's init kit), `track`
(the selected track's steps), `pattern` (all tracks), `autom. 1` / `autom. 2` (the selected track's automation values).
Confirm with COPY/CLR or the DATA push. Turning SHIFT off cancels.

---

## 10 The mod: RESET and the randomizers

**RESET** (blue key, top left): back to the factory project (the Hrtl LXR-02 DLC presets), pattern 1 with its kit,
the sequencer stopped, the FX tails cut. Anything not saved is replaced.

**Factory content**, old and new presets side by side, and 16 demo patterns, each with its kit:

- kits 1-16: the Hrtl kits (Lambo, Reservoir, Faders, Malware, Zipper, Nanobyte, Kronky, Boooost, Dropbar, Goondam,
  SpaceD, Penalty, Dialup, Express, Necro, Mist) as the LXR-02 plays them;
- kits 17-32: the same kits as LXRXXL 0.5 shipped them ("Lambo05" ... "Mist05");
- kit 33 "Init 05": LXRXXL 0.5's init kit; kit 34 "Initkit": the LXR-02 firmware's init kit; 35-64: Initkits.

A new LXRXXL starts on them; a saved MPC project opens as you saved it (also one from 0.5.x: its kits are moved to the
LXR-02 layout on load).

Along the top: two keys, each showing its knob's amount, and two knobs (0-100 % in steps of 10). The keys show their
colour and light up brighter when pressed.

**Sound randomizer** (green key with the warning sign, green knob): every sound parameter of the six voices moves up or
down by a random amount, up to the knob's share of its range, each on its own draw. A parameter never lands where it
is, nor back where it was before the last press. Anything that could silence a voice keeps at least 30 %: the voice
volumes, the amp decays, the oscillator pitch and a filter's cutoff (a high-pass cutoff at most 70 %, an attack at
most 70 %). The outputs and the modulation routings stay. **SHIFT + green key**: only the selected voice.

**Step randomizer** (yellow key with the question mark, yellow knob): a new beat on the pattern, a euclidean rhythm per
voice. At n % each voice gets about n % of its track's steps (between half and all of that, drawn per voice), spread
evenly, turned by a random rotation, with random accents. Each press is a new beat; it replaces the steps. **SHIFT +
yellow key**: only the selected voice, at the knob's density.

The two knobs are on Q-Link bank 2 (knobs 5 and 6); MPC's project keeps their settings.

---

## 11 Global settings (CONFIG)

**SHIFT + CONFIG**: the display shows `Global`.

| Value | What |
|---|---|
| `bpm` | tempo: `MPC` (follow MPC) or 1-255 ([2](#2-playing-and-the-transport)) |
| `ch` | the global MIDI channel: notes on it play the selected voice tonally |
| `mtx` | MIDI transmit filter (kept with the project; the plugin sends no MIDI) |
| `mrx` | MIDI receive filter: which kinds of MIDI the LXR takes, N (notes), C (control changes), P (program changes), R, in combinations (`all` = everything, `off` = nothing) |
| `flw` | follow (kept with the project) |
| `kcm` | kit change mode: `Aut` = a pattern brings its kit, `off` = the kit stays |
| `pad` | the Force's pads: `nte` = they play the voices, `pnl` = they are the panel (after pad learn) |
| `lrn` | pad learn ([12](#12-force-pads-q-links-and-midi)) |

---

## 12 Force pads, Q-Links and MIDI

### 12.1 Pads play the voices (default)

Pad 1 (bottom left) = DRUM1, 2 = DRUM2, 3 = DRUM3, 4 = SNARE, 5 = CLP/CYM, 6 = CL HH, 7 = OP HH; pads 8-16 are silent.
This works with the Force's default pad layout (C major from C1: notes 36, 38, 40, 41, 43, 45, 47) and with the
drum-pad patch (pad n sends note n − 1). With another layout or key, set each voice's note on its MIX page (`nte`).
With MPC stopped, the display shows what each hit played (`note 36 > DRUM1`).

**Sequencing from the Force's step sequencer:** MPC's grid plays the voices in that order. MPC's play also plays the
LXR's own pattern: to sequence only from the Force, clear the pattern (SHIFT + COPY/CLR > pattern) or pick an empty one
(17-64).

### 12.2 Pads as the panel (pad learn)

Once per device: CONFIG > `lrn`, then hit three pads: the bottom-left pad, the pad right of it, the pad above it. The
display shows their notes, then `Pads learned`. The pads are then the LXR's keys, row by row from the bottom:

| Row | Pads 1-8 |
|---|---|
| 6 | – – – – – – – OP HH |
| 5 | BAR◀ BAR▶ DRUM1 DRUM2 DRUM3 SNARE CLP/CYM CL HH |
| 4 | OSC AEG MOD FM CLICK FILTER LFO MIX |
| 3 | SHIFT PLAY REC COPY/CLR VOICE PERF LOAD SAVE |
| 2 | steps 9-16 |
| 1 | steps 1-8 |

On pads, keys work held (SHIFT, COPY, rolls, holding a step for parameter locks). The layout must send different notes
on every pad; overlapping rows give `Pads overlap` and the pads keep playing the voices. CONFIG > `pad` switches
between `nte` and `pnl`. A plugin can't light the Force's pads: the LEDs are on the screen. The DATA push during pad
learn cancels it.

### 12.3 Q-Links

- **Bank 1:** knobs 1-7 = the 7 faders, knob 8 = DATA.
- **Bank 2:** knobs 1-4 = the 4 pots, 5 = sound randomizer amount, 6 = step randomizer amount, 8 = DATA.

### 12.4 MIDI

- Each voice listens on its own channel and note (MIX > `ch`, `nte`; by default channel 1, notes 36-42).
- The global channel (CONFIG > `ch`) plays the selected voice at the note's pitch.
- CC n sets sound parameter n − 1; NRPN sets the parameters from 128 (the LXR's CC/NRPN list); NRPN 100-105 the pulse
  width of voices 1-6 and 106-117 the FX unit (reverb and delay type included); CC 1 morphs; program change selects the pattern.

---

## 13 Files and projects

- **Projects** (LOAD / SAVE > PROJECT) are saved in `/sdcard/LXRXXL/Projects`, outside the plugin folder, so updates
  keep them.
- **MPC's project** keeps each LXRXXL's whole state (kits, patterns, songs, settings, randomizer amounts). Open the MPC
  project and the LXR is as you left it.
- The pad learn setting is kept per device in `/sdcard/LXRXXL/settings.cfg`.

---

## 14 Quick reference

| Do this | How |
|---|---|
| Change kit | LOAD, KIT, DATA to the kit, tap DATA (or KIT) |
| Change pattern | SHIFT + PATTERN, BAR for the group, step key |
| Save the kit | SAVE, KIT, DATA to the slot, confirm, name, `ok` |
| Edit a sound | VOICE, voice key, page key, pots / rows |
| Values 5-8 of a page | page key again |
| Enter steps | VOICE, voice key, BAR, step keys |
| Step velocity / probability | VOICE twice (step mode), step key, `vel` / `prb` |
| Track length | SHIFT + LAST STEP, step key (twice = all tracks) |
| Mute | SHIFT + voice key (or PERF + voice key) |
| Rolls | PERF, steps 1-7 |
| Copy a track | COPY/CLR, source voice key, destination voice key |
| Clear | SHIFT + COPY/CLR, DATA, COPY/CLR |
| Effect | SHIFT + FX |
| Tempo | SHIFT + CONFIG > `bpm` (`MPC` or a BPM) |
| Random sounds | green key (SHIFT: one voice), green knob = amount |
| Random beat | yellow key (SHIFT: one voice), yellow knob = density |
| Back to the presets | RESET |
| Pads as the panel | SHIFT + CONFIG > `lrn`, three pads |

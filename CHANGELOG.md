# Changelog

## 0.5.10
- Built on the latest wrapper and includes the Q-Link and automation fix (mpc-vst-plugins #486-#489): Q-Links now
  move this plugin's parameters on the track's automation screen (MPC no longer rebuilds its parameter screen on every
  write, at most one refresh per 50 ms), and the build keeps every parameter's index fixed (`params.lock`), so saved
  projects, automation and Q-Link maps stay on the right parameters in later versions. Nothing else changed.
- Packaged with the fixed L'Cronx release tools (the ' in the plugin folder name is escaped in install.sh and
  uninstall.sh, and the catalog check no longer flags it). Installs over 0.5.9 in place: no need to uninstall first.

## 0.5.9
- Packed under **L'Cronx**: the plugin folder is now `L'Cronx - VST - LXRXXL` and MPC lists the maker as L'Cronx. Nothing else
  changed from 0.5.8. If you installed 0.5.8, run its `uninstall.sh` first (copy out anything you saved inside the plugin
  folder), then install 0.5.9. The installer handles the ' in the folder name.

## 0.5.8
- Bigger text, sizes and places unchanged: the PARAMETERS column's names and values (30), the display's two lines
  (30), the pots' values (24) and the randomizer amounts on the green and yellow keys (15).

## 0.5.7
- The new panel by Suns Kiefer (art/panel/PANEL SOURCE.png, fitted under the keys by tools/fit_panel.py), with the
  top bezel; the keys, pots, faders, the data encoder and RESET keep their places and pictures, and the DATA dial is
  centred on the encoder.
- The randomizers move to the top of the panel with new knob caps and keys (green with a warning sign, yellow with a
  question mark); each key shows its knob's amount. RESET, green and yellow keys show their colour unlit and brighten
  when lit.

## 0.5.6
- LOAD / SAVE: the "DATA" legend (the encoder's push) blinks red while a slot waits to be confirmed (also in CLEAR and
  pad learn), and pressing the same data key again (KIT, MORPH, SONG, PROJECT) confirms too. Changing kit: LOAD, KIT,
  turn DATA, tap DATA (or KIT again). The README says how; the Force's data wheel press does not reach plugins.

## 0.5.5
- The panel without the LXR logo, the "LXR-02" text and the "X SONIC POTIONS X ERICA SYNTHS" line ("DIGITAL DRUM
  SYNTH" and the mod mark stay); the page tab and Q-Link page are named LXRXXL.
- README, catalog and release text say what it is: a port of the open-source Sonic Potions LXR engine with
  LXR-02-style features written from the manual, inspired by the Erica Synths LXR-02.

## 0.5.4
- The panel without the L'Cronx logo (new background by Suns Kiefer); the catalog entry's author is Suns Kiefer.

## 0.5.3
- The DATA knob turns only when DATA is turned: a pot selecting its value no longer moves it.
- RANDOM: a filter cutoff moved back into its safe range never lands where it was before the last press either.
- Tests: the randomizer checks look at whole tracks (up to 64 steps) and give a slow attack a second to sound.

## 0.5.2
- The DATA encoder's new knob (matte, by Suns Kiefer).

## 0.5.1
- PARAMETERS column: bigger names and values (25 px, was 17) and heading.

## 0.5.0
- Factory presets: the Hrtl LXR-02 DLC project (64 kits, 16 demo patterns with their kits), imported from the LXR-02's
  own files by tools/import_lxr02.py. A new instance starts on it; a saved MPC project opens as saved.
- RESET key (blue): back to the factory project.
- SHIFT + RANDOM / SHIFT + SEQ RND: only the selected voice.
- The page: the PARAMETERS column narrower (188 px), the panel widened to fill the rest; the mod keys with their icons
  (⚠ random, ? step random) and the RESET key and legend.

## 0.4.0 (LXR-02 mod)
- The modded panel (artwork by Suns Kiefer): a voice randomizer (green key + amount knob, 10 % steps) and a step randomizer (yellow
  key + density knob, euclidean per voice), with their scales and legends on the panel; the knobs on Q-Link bank 2.
- Randomizing never silences a voice (volumes, decays, pitch, cutoffs kept in their sounding range), never lands a
  parameter back where it was, and moves every voice on its own draws. Tests: test/engine_test.c "randomizers".

## 0.3.1
- The Force's pads play the voices in order. 0.3.0 put the voices on notes 36-42, but the Force's pads send the C major
  scale (36 38 40 41 43 45 47 ...), so pads 1-4 played DRUM1, DRUM3, CLP/CYM, CL HH and the rest nothing. The voices'
  default notes are now those pad notes; an MPC project saved with 0.1-0.3 is moved over when it loads (voices still on
  the old defaults).

## 0.3.0
- MPC's step sequencer drives the LXR: the drum pads play the voices in the LXR's order (pad 1 = DRUM1 ... pad 7 =
  OP HH), from MPC's pad bank A (notes 36-42) or the drum-pad patch layout (notes 0-6); pads 8-16 are silent instead of
  playing the selected voice. With MPC stopped the display shows what a pad played.
- Patterns start empty (pattern 1 had a starter beat that played over MPC's steps).
- The pots stand on the value they edit and turn from its minimum to its maximum; the DATA knob turns with the
  selection, from the page's first value to its last. MPC's Q-Links and wheel step them as before.

## 0.2.1
- Sound: MPC's song position and play state never reached the sequencer (the wrapper only asked MPC for its time with
  other options set), so MPC's play did nothing. Fixed, and checked by a new test that drives the plugin through the
  wrapper as MPC does (`test/mpc_sim.c`), also built for ARM and run under qemu (`test/arm_sim.sh`).
- The LXR's own PLAY works with the tempo on "MPC" when MPC is stopped (it showed a message before), and its key
  stays lit while the LXR runs.
- Pads: 0.1 assumed the pads send notes 36 up in rows of 8 and took those notes as panel keys, so pads made no sound and
  pressed the wrong keys. Now nothing is assumed: until pad learn the pads play the drums; pad learn takes three pads
  (bottom-left, right of it, above it) so any non-overlapping layout maps, and refuses an overlapping one. A layout
  saved by 0.1 is ignored.

## 0.2.0
- The page is the LXR-02 panel artwork by Suns Kiefer: its keys (lit red with their LEDs), pots, data encoder and fader caps,
  at MPC's full height. The page's values moved to a PARAMETERS column at its left (name, value, a bar to drag); the
  pots print the value they edit in their white centre; the encoder's push is a tap on its DATA legend.

## 0.1.0
- First version: the LXR-02 on one page (display, pots, encoder, faders, all keys with their LEDs, step keys red/white),
  the LXR's voices from LXR02Open, the LXR-02's PWM waveform and FX unit, the sequencer with 64 patterns of 4 bars,
  songs, rolls, recording, the pattern generator and kit morph; Force pads as the panel (pad learn); MPC tempo follow;
  Q-Link banks: faders + encoder, pots + encoder.

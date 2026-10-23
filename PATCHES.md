# Changes to the vendored LXR firmware

`third_party/lxr-desktop-app/` is the Erica Synths LXR firmware ([erica-synths/lxr-desktop-app](https://github.com/erica-synths/lxr-desktop-app),
GPL-3.0-only, the commit in `VENDORED_COMMIT`), only the parts the sound engine needs: `Core/Inc` and, from `Core/Src`,
the DSP (`Soundengine/DSPAudio`, its `FX` folder), the FX bus (`Soundengine/FxBus.c`), the parameter table
(`Soundengine/MIDI/ParameterArray.c`), the parameter order (`Parameters.c`), the modulation target list
(`menu/Cc2Text.c`) and the init kit (`Preset/initKit.c`). `Soundengine/MIDI/MidiParser.c` and `menu/menu.c` are there
for `tools/extract_cc.py`, which copies the parameter handler (`src/lxr/cc_handler.inc`) and the menu's value types and
names (`src/lxr/menu_tables.inc`) out of them verbatim. Everything else (the sequencer, the front panel, storage, the
plugin) is LXRXXL's own code under `src/`.

Changed files, each change marked `LXRXXL` in the source:

- `Core/Src/Soundengine/DSPAudio/Oscillator.c`: `calcNoise()` read the ARM's overflow flag (APSR V) to see the phase
  wrap; it compares the phase with the previous one instead.
- `Core/Src/Soundengine/DSPAudio/lfo.c`: tempo-synced LFOs take the tempo as a float (`lxr_dsp_getBpm()`), so MPC's
  fractional tempos stay in step (the firmware's `seq_getBpm()` is whole BPM).
- `Core/Src/Soundengine/DSPAudio/modulationNode.c`: `modNode_setOriginalValueChanged()` skips a destination without a
  parameter (slot 0, whose pointer is NULL: reading address 0 is harmless on the STM32, a crash here).
- `Core/Src/Soundengine/DSPAudio/FX/Delay.c`: the ping-pong delay masks its pointers to its half of the buffer before
  it runs. Right after switching from the plain delay they could still point past it, reading and writing past
  `FX_BUFFER`.
- `Core/Inc/Soundengine/FxBus.h`: `USE_REVERB 1`. The firmware has the plate reverb (`FX/Reverb.c`, its menu page and
  parameter) but ships with it off; LXRXXL turns it on as FX type 6. As in the firmware's code, the snare, clap/cymbal
  and hi-hat filters use the cheaper filter while the reverb runs.
- `Core/Inc/menu/MenuText.h`: "rev" in the FX type list (`fxNames`), which the firmware has commented out.

Not vendored, replaced by `src/v2shim/` (hardware only): the STM32 HAL headers (`stm32f7xx_hal.h`, the Cortex-M
intrinsics `__SSAT`, `__QADD16`, `__PKHBT`... as portable C), `main.h` (the output jack switches: all plugged),
`IO/adcPots.h` (the faders, set by the plugin), `delay.h`, `Soundengine/DSPAudio/random.h` and `random.c` (the STM32's
hardware RNG: a xorshift generator per instance), and `Soundengine/config.h` (the firmware's own, with the sample rate
at 44100 instead of 44140 and no TCM memory sections).

Every engine file is compiled with `-include src/shim/lxr_state.h`, which puts its globals in sections
`src/lxr_state.c` swaps per plugin instance.

What the plugin does around the engine (`src/lxr/lxr_dsp.c`): kits are sent the way `preset_sendDrumsetParameters()`
sends them (the mod targets as parameters, then every parameter in `parameterOrder`), values are kept inside each
parameter's range as the menu keeps them, the mixer's two stereo outputs are added into MPC's track output, and a DC
blocker stands in for the outputs' coupling capacitors.

# The source lists, shared by build.sh and test/run_tests.sh.
V=third_party/lxr-desktop-app/Core
# the LXR engine: the Erica Synths LXR firmware's sound engine (GPL-3.0). Its globals become per-instance state:
# compiled with -include src/shim/lxr_state.h.
ENGINE_SRCS="$(ls $V/Src/Soundengine/DSPAudio/*.c $V/Src/Soundengine/DSPAudio/FX/*.c $V/Src/Soundengine/DSPAudio/FX/VerbTools/*.c) $V/Src/Soundengine/FxBus.c $V/Src/Soundengine/MIDI/ParameterArray.c $V/Src/menu/Cc2Text.c $V/Src/Parameters.c $V/Src/Preset/initKit.c src/v2shim/random.c src/lxr/lxr_dsp.c"
# the plugin
PLUGIN_SRCS="src/meta.c src/lxr.c src/seq.c src/ui.c src/random.c src/factory_data.c src/factory_05.c src/store.c src/lxr_state.c src/engine.c"
# src/v2shim first: it stands in for the hardware headers (HAL, pins, pots, RNG) and the firmware's config.h
INCS="-Isrc -Isrc/lxr -Isrc/v2shim -I$V/Inc -I$V/Inc/menu -I$V/Inc/Soundengine/MIDI"

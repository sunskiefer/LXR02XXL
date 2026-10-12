#!/usr/bin/env python3
"""LXRXXL's one page: where every part of the LXR-02 panel sits on MPC's plugin screen.

    python3 tools/panel.py --write     (writes layout.conf from this file)
    python3 tools/panel.py --check     (fails when layout.conf is stale)

The page is built from the panel artwork in art/panel/ (the panel by Suns Kiefer, laid out like the LXR-02, 1000 x 638):
    BACKGROUND SYNTH.png                   the panel without its knobs, keys and fader caps (made from PANEL SOURCE.png,
                                           fitted to the keys' places by tools/fit_panel.py)
    BUTTONS OFF ALL.png / BUTTONS ON ALL.png   every key unlit / lit, in place (a sheet 8 px left and 1 px up of the panel)
    Main KNOB.png                          the data encoder
    PARAMETER DATA KNOB.png                the four parameter pots
    SLIDER CAP one.png                     a fader cap
The panel is scaled to MPC's 628 px height and sits at the right; the column at its left is the page's eight values
(the LXR's menu page, which the display, the pots and the encoder edit) as horizontal sliders with their names.

Everything here is in the artwork's pixels and converted with page() / layout_y(); the keys are found in the key
sheets themselves, so new artwork with the keys elsewhere needs no change here. Coordinates in layout.conf are Force
Shadow pixels: the plugin area is x 0..1280, y 86..714 (mpc-vst-plugins tools/shadow_skin.py)."""
import os
import sys

import numpy as np
from PIL import Image
from scipy import ndimage

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ART = os.path.join(ROOT, "art", "panel")
sys.path.insert(0, os.path.join(ROOT, "tools"))
import gen_params  # noqa: E402

Y0 = 86
W, H = 1280, 628

# ---- the artwork and where it goes
ART_W, ART_H = 1000, 638
OX = 188                           # the PARAMETERS column is 0..188 (PAGE REFERENCE.png), the panel the rest
SX = (W - OX) / ART_W              # 1.092: the panel is widened to fill the page, as in the reference
SY = H / ART_H                     # 0.984: and at MPC's full height
S = SY                             # round parts (knobs) keep their shape: scaled by SY only
SHEET_DX, SHEET_DY = 8, 1          # the key sheets' offset into the panel (measured against ALL OVERVIEW.png)


def page(x, y):
    """artwork pixels -> page pixels (0..1280, 0..628)"""
    return OX + x * SX, y * SY


# ---- the panel's parts, in artwork pixels
LCD = (84, 147, 280, 79)           # the black display window: x, y, w, h
DATA = (465.5, 182.5)              # the data encoder's centre (Main KNOB.png, 101 x 105)
POTS = [(71.5, 332.5), (172.5, 332.5), (273.5, 332.5), (374.5, 332.5)]   # PARAMETER DATA KNOB.png, 93 x 93
POT_DISC = 47                      # the white centre of the pot picture (diameter): the value is printed there
ENC = (465.5, 262, 64, 22)         # the encoder's push: a touch area on the "DATA" legend (cx, cy, w, h)

# ---- the mod: the voice randomizer (green) and the step randomizer (yellow), each a key (MOD BUTTON GREEN / YELLOW ON.png;
# the unlit white is made from it) and an amount knob (MOD KNOB GREEN / YELLOW.png), in the band above the display and the
# faders, with their legends (MOD LABEL VOICE.png, the warning sign; MOD LABEL STEP.png, the question mark). Keys and knobs
# are round on the page: key_w and knob_size are their page size / S. label: picture, top-left, page px per picture px.
MOD = {
    "rnd": dict(key=(598.2, 27.3), key_w=49.0, key_img="MOD BUTTON GREEN ON.png", knob=(521.6, 53.9), knob_size=99.9,
                knob_img="MOD KNOB GREEN.png", label=("MOD LABEL VOICE.png", 621.5, 1.3, 1.13), amt="rnd_amt", round=True),
    "srnd": dict(key=(323.9, 27.9), key_w=49.0, key_img="MOD BUTTON YELLOW ON.png", knob=(401.5, 53.9), knob_size=99.9,
                 knob_img="MOD KNOB YELLOW.png", label=("MOD LABEL STEP.png", 278.9, 5.0, 1.13), amt="srnd_amt", round=True),
}
RESET_KEY = dict(key=(54.0, 76.5), key_w=50, key_img="MOD BUTTON BLUE ON.png")   # the blue RESET key
RESET_LABEL = ("MOD RESET LABEL.png", 4, 26)   # its legend, cut from MOD OVERVIEW.png at this place
MOD_KNOB_DISC = 34                 # the knob's black centre (diameter, in knob_size units): the amount is printed there
FADERS = [586.5, 648.5, 709.5, 769.5, 833.0, 893.5, 955.5]   # fader centres (x)
FADER_TOP, FADER_BOTTOM = 96, 317  # the fader frame: the cap's travel
FADER_W = 33                       # the cap's width (SLIDER CAP one.png, 33 x 51)

# ---- the keys, found in the key sheet: three rows, sorted left to right
ROW_VOICE = ["barl", "barr", "v1", "v2", "v3", "v4", "v5", "v6", "v7"]
ROW_FUNC = ["shift", "play", "rec", "copy", "voice", "perf", "load", "save",
            "osc", "aeg", "mod", "fm", "click", "filter", "lfo", "mix"]
ROW_STEP = ["s%d" % (i + 1) for i in range(16)]


def find_keys():
    """{key: (x, y, w, h)} in artwork pixels, from the alpha of BUTTONS OFF ALL.png"""
    a = np.array(Image.open(os.path.join(ART, "BUTTONS OFF ALL.png")).convert("RGBA"))[:, :, 3] > 20
    lab, n = ndimage.label(a)
    boxes = [(s[1].start, s[0].start, s[1].stop - s[1].start, s[0].stop - s[0].start) for s in ndimage.find_objects(lab)]
    boxes = [b for b in boxes if b[2] > 20 and b[3] > 20]
    rows = []
    for b in sorted(boxes, key=lambda b: b[1] + b[3] / 2):   # a new row where the centres jump by more than 20 px
        if rows and abs(b[1] + b[3] / 2 - (rows[-1][-1][1] + rows[-1][-1][3] / 2)) <= 20:
            rows[-1].append(b)
        else:
            rows.append([b])
    rows = [sorted(r) for r in rows]
    want = [ROW_VOICE, ROW_FUNC, ROW_STEP]
    if [len(r) for r in rows] != [len(w) for w in want]:
        raise SystemExit("panel: the key sheet has rows of %s keys, expected 9, 16, 16" % [len(r) for r in rows])
    out = {}
    for keys, r in zip(want, rows):
        for k, (x, y, w, h) in zip(keys, r):
            out[k] = (x + SHEET_DX, y + SHEET_DY, w, h, x, y)   # panel x, y, w, h + where it is in the sheet
    return out


KEYS = find_keys()

# ---- the value column (page pixels)
COL = (12, 12, OX - 24, H - 24)    # the screen holding the eight rows
ROW_Y0, ROW_H = 62, 68             # first row's top, row pitch
ROW_X, ROW_W = 24, OX - 48         # each row's box
BAR_H = 20                         # the slider bar under the name and value


def row_box(i):
    return (ROW_X, ROW_Y0 + ROW_H * i, ROW_W, ROW_H - 8)


def layout_lines():
    def L_y(y):
        return round(y + Y0)

    L = ["# LXRXXL -- page layout (mpc-vst-plugins layout.conf syntax: tools/shadow_skin.py, docs/SKIN_STUDIO.md).",
         "# GENERATED by tools/panel.py: edit that, then run `python3 tools/panel.py --write`. tools/panel_art.py then",
         "# replaces the drawn controls with the panel artwork (art/panel/) at the same places.",
         "theme_bg=141414", "theme_panel=141414", "theme_ink=ececec", "theme_ink_dim=cfcfcf", "theme_accent=f2f2f2",
         "theme_knob_ring=e2201c", "theme_seg_active=e2201c", "theme_lcd=000000", "theme_box=000000",
         "", "[tab LXRXXL]"]
    x, y, w, h = LCD
    cx, cy = page(x + w / 2, y + h / 2)
    lw = round(w * SX) - 8
    lh = round(h * SY / 2) - 2
    L.append('readout cx=%d cy=%d w=%d h=%d label="" key=lcd1' % (round(cx), L_y(cy - lh / 2 - 1), lw, lh))
    L.append('readout cx=%d cy=%d w=%d h=%d label="" key=lcd2' % (round(cx), L_y(cy + lh / 2 + 1), lw, lh))
    for i, (px, py) in enumerate(POTS):
        cx, cy = page(px, py)
        L.append('knob cx=%d cy=%d r=40 label="" key=pot%d ns=0 bw=92' % (round(cx), L_y(cy), i + 1))
    cx, cy = page(*DATA)
    L.append('knob cx=%d cy=%d r=48 label="" key=data ns=0 bw=104' % (round(cx), L_y(cy)))
    for i in range(8):
        x, y, w, h = row_box(i)
        L.append('slider_h cx=%d cy=%d w=%d h=%d label="" key=slot%d ns=0 bw=%d when=slot%d_used:on'
                 % (x + w // 2, L_y(y + h / 2), w, BAR_H, i + 1, w, i + 1))
    for i, fx in enumerate(FADERS):
        cx, cy = page(fx, (FADER_TOP + FADER_BOTTOM) / 2)
        L.append('slider_v cx=%d cy=%d w=%d h=%d label="" key=fader%d ns=0 bw=40'
                 % (round(cx), L_y(cy), round(FADER_W * SX), round((FADER_BOTTOM - FADER_TOP) * SY), i + 1))
    for k, _ in gen_params.BUTTONS:
        if k == "enc":
            cx, cy = page(ENC[0], ENC[1])
        else:
            x, y, w, h = KEYS[k][:4]
            cx, cy = page(x + w / 2, y + h / 2)
        L.append('button cx=%d cy=%d label="" key=%s' % (round(cx), L_y(cy), k))
    cx, cy = page(*RESET_KEY["key"])
    L.append('button cx=%d cy=%d label="" key=reset' % (round(cx), L_y(cy)))
    for k, m in MOD.items():
        cx, cy = page(*m["key"])
        L.append('button cx=%d cy=%d label="" key=%s' % (round(cx), L_y(cy), k))
        cx, cy = page(*m["knob"])
        L.append('knob cx=%d cy=%d r=44 label="" key=%s ns=0 bw=96' % (round(cx), L_y(cy), m["amt"]))
    # Q-Links: bank 1 = the 7 faders and the data encoder on knob 8; bank 2 = the 4 pots and the encoder on knob 8
    L.append('qlinks "LXRXXL" = fader1,fader2,fader3,fader4,fader5,fader6,fader7,data,pot1,pot2,pot3,pot4,rnd_amt,srnd_amt,-,data')
    return L


if __name__ == "__main__":
    text = "\n".join(layout_lines()) + "\n"
    if "--write" in sys.argv:
        open(os.path.join(ROOT, "layout.conf"), "w").write(text)
        print("layout.conf written")
    elif "--check" in sys.argv:
        if open(os.path.join(ROOT, "layout.conf")).read() != text:
            raise SystemExit("layout.conf is stale: run python3 tools/panel.py --write")
    else:
        sys.stdout.write(text)

#!/usr/bin/env python3
"""Make the panel background (art/panel/BACKGROUND SYNTH.png) from the panel artwork (art/panel/PANEL SOURCE.png).

    python3 tools/fit_panel.py

PANEL SOURCE.png (Suns Kiefer, 950 x 563) is the panel with its keys drawn in, some lit, and the knob places cut out.
The keys, faders and knobs of the page keep their places (tools/panel.py): the artwork is fitted under them.
  - the lit keys are drawn unlit, from the unlit key next to them in the same row (the page draws the lights);
  - the knob holes take the panel's dark (the knobs cover them);
  - the artwork is mapped onto the 1000 x 638 panel: panel x = (art x - 8.4) / 0.937, panel y = (art y - 59.7) / 0.783,
    measured on the keys (three rows, 41 keys) and the data encoder. Above the display the band from art y 59.7 on is
    kept (the sockets' legends above it are left out: the RESET key and the randomizers sit there)."""
import os

import cv2
import numpy as np
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ART = os.path.join(ROOT, "art", "panel")
KX, OX, KY, OY = 0.937, 8.4, 0.783, 59.7
SMALL_R, BIG_R = 16, 23
# (lit key centre, the unlit key it is redrawn from, radius), in PANEL SOURCE.png pixels
UNLIGHT = [((792.5, 342.5), (734.5, 342.5), SMALL_R),   # CLP/CYM (voice row)
           ((272.5, 402.5), (330.5, 402.5), SMALL_R),   # VOICE
           ((157.5, 460.5), (99.5, 460.5), BIG_R),      # DRUM3
           ((446.5, 459.5), (388.5, 459.5), BIG_R),     # KIT
           ((561.5, 459.5), (619.5, 458.5), BIG_R),     # SONG
           ((734.5, 458.5), (503.5, 459.5), BIG_R),     # RELOAD (white frame), from MORPH
           ((908.5, 457.5), (850.5, 457.5), BIG_R)]     # PATTERN


def redraw(img, dst, src, r):
    h, w = img.shape[:2]
    yy, xx = np.mgrid[0:h, 0:w]
    m = np.clip((r + 1.5 - np.hypot(xx - dst[0], yy - dst[1])) / 3, 0, 1)[..., None]
    shift = np.float32([[1, 0, src[0] - dst[0]], [0, 1, src[1] - dst[1]]])
    moved = cv2.warpAffine(img, shift, (w, h), flags=cv2.INTER_LINEAR | cv2.WARP_INVERSE_MAP)
    return img * (1 - m) + moved * m


def main():
    a = np.asarray(Image.open(os.path.join(ART, "PANEL SOURCE.png")).convert("RGBA")).astype(np.float32)
    for dst, src, r in UNLIGHT:
        a = redraw(a, dst, src, r)
    alpha = a[..., 3:] / 255
    a = a * alpha + np.array([22, 22, 25, 255], np.float32) * (1 - alpha)
    a[..., 3] = 255
    fit = np.float32([[KX, 0, OX], [0, KY, OY]])   # panel -> artwork
    out = cv2.warpAffine(a, fit, (1000, 638), flags=cv2.INTER_CUBIC | cv2.WARP_INVERSE_MAP, borderMode=cv2.BORDER_REPLICATE)
    Image.fromarray(np.clip(out, 0, 255).astype(np.uint8)).save(os.path.join(ART, "BACKGROUND SYNTH.png"))
    print("fit_panel: BACKGROUND SYNTH.png written")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Put the LXR-02 panel artwork (art/panel/) into the skin mpc-vst-plugins built from layout.conf.

    python3 tools/panel_art.py "<skin dir>/Plugin Skins"

Geometry: tools/panel.py. What it does to the built skin:
  - the page background (sh_bg_0.png): the panel (BACKGROUND SYNTH.png) at full height on the right, and at its
    left the value column: a black screen with a row per value of the LXR's menu page;
  - every key gets its own two pictures, cut from the key sheets (BUTTONS OFF ALL.png = unlit, BUTTONS ON ALL.png =
    lit) over the piece of panel under it, at its exact place: the LED shows as the lit picture;
  - the faders are filmstrips of SLIDER CAP one.png along the panel's fader frames, the four pots and the data encoder
    filmstrips of PARAMETER DATA KNOB.png / Main KNOB.png turning over the panel; the pot's value is printed in its
    white centre;
  - the value rows: the value's name on the left, its value on the right, a bar to drag under them;
  - the two display lines are MPC's live text in the panel's display window.
Each control's definition in TUI.json is replaced by one with exactly those pictures and bounds; definitions and
images nothing uses any more are removed."""
import json
import math
import os
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import panel as P  # noqa: E402

FONT = os.path.join(P.ROOT, "art", "fonts", "TitilliumWeb-SemiBold.ttf")
LCD_INK = "ffe8f4ff"
ROW_INK, ROW_DIM = "fff2f2f2", "ff9a9a9a"
MAX_STRIP = 12288
KNOB_FRAMES = 128
SS = 3


def art(name):
    return Image.open(os.path.join(P.ART, name)).convert("RGBA")


def font(px):
    return ImageFont.truetype(FONT, max(1, int(round(px))))


def scaled(img, kx=P.SX, ky=P.SY):
    return img.resize((max(1, round(img.size[0] * kx)), max(1, round(img.size[1] * ky))), Image.LANCZOS)


# ------------------------------------------------------------------------------------------------ the background
def draw_background():
    bg = Image.new("RGBA", (P.W, P.H), (20, 20, 21, 255))
    # the column: the panel's dark finish (a fine noise over a soft vertical gradient)
    rng = np.random.default_rng(7)
    g = np.linspace(30, 20, P.H)[:, None] + rng.normal(0, 3.0, (P.H, P.OX))
    col = np.clip(np.repeat(g[:, :, None], 3, axis=2), 0, 255).astype(np.uint8)
    bg.paste(Image.fromarray(col, "RGB"), (0, 0))
    panel = art("BACKGROUND SYNTH.png")
    # the mod: the RESET legend printed on the panel
    name, lx, ly = P.RESET_LABEL
    panel.alpha_composite(art(name), (lx, ly))
    panel = scaled(panel)
    bg.alpha_composite(panel, (P.OX, 0))
    # the randomizers' legends, at their own proportions on the page
    for m in P.MOD.values():
        name, lx, ly, k = m["label"]
        pic = art(name)
        pic = pic.resize((round(pic.size[0] * k), round(pic.size[1] * k)), Image.LANCZOS)
        px, py = P.page(lx, ly)
        bg.alpha_composite(pic, (round(px), round(py)))
    d = ImageDraw.Draw(bg)
    # the screen of the value column, in the style of the panel's display window
    x, y, w, h = P.COL
    d.rounded_rectangle([x - 3, y - 3, x + w + 3, y + h + 3], radius=8, fill=(44, 44, 46, 255))
    d.rounded_rectangle([x, y, x + w, y + h], radius=6, fill=(0, 0, 0, 255))
    d.text((x + 14, y + 12), "PARAMETERS", font=font(18), fill=(230, 230, 230, 255))
    d.line([(x + 14, y + 40), (x + w - 14, y + 40)], fill=(70, 70, 72, 255), width=1)
    # the seam between the column and the panel
    d.line([(P.OX - 1, 0), (P.OX - 1, P.H)], fill=(8, 8, 8, 255), width=2)
    return bg


def crop(bg, x, y, w, h):
    return bg.crop((x, y, x + w, y + h)).convert("RGBA")


# ------------------------------------------------------------------------------------------------ the keys
def mod_key_pictures(bg, m):
    """the mod's keys (green, yellow, blue): lit = the key's own picture, unlit = the same glass in white"""
    src = art(m["key_img"])
    k = m["key_w"] / src.size[0]
    if m.get("round"):   # the randomizer keys: round on the page
        w, h = round(src.size[0] * k * P.S), round(src.size[1] * k * P.S)
    else:
        w, h = round(src.size[0] * k * P.SX), round(src.size[1] * k * P.SY)
    cx, cy = P.page(*m["key"])
    x0, y0 = round(cx - w / 2), round(cy - h / 2)
    a = np.array(src).astype(np.float32)
    rgb, alpha = a[..., :3], a[..., 3:]
    lum = rgb @ np.array([0.30, 0.59, 0.11], np.float32)
    white = np.repeat(np.clip(lum * 1.15 + 70, 0, 255)[..., None], 3, axis=2)   # the same glass, unlit
    lit = rgb
    pics = []
    for col in (white, lit):
        g = Image.fromarray(np.concatenate([col, alpha], axis=2).astype(np.uint8), "RGBA")
        im = crop(bg, x0, y0, w, h)
        im.alpha_composite(g.resize((w, h), Image.LANCZOS))
        pics.append(im)
    return pics[0], pics[1], (x0, y0, w, h)


def key_pictures(bg, key, sheets):
    """(off, on) pictures and the page rectangle of a key"""
    if key == "enc":
        cx, cy, w, h = P.ENC
        px, py = P.page(cx - w / 2, cy - h / 2)
        x, y, w, h = round(px), round(py), round(w * P.SX), round(h * P.SY)
        off = crop(bg, x, y, w, h)
        on = off.copy()
        glow = Image.new("RGBA", (w, h), (0, 0, 0, 0))
        ImageDraw.Draw(glow).rounded_rectangle([1, 1, w - 2, h - 2], radius=h // 2, outline=(226, 40, 36, 255), width=2)
        on.alpha_composite(glow)
        return off, on, (x, y, w, h)
    ax, ay, aw, ah, sx, sy = P.KEYS[key]
    px, py = P.page(ax, ay)
    x0, y0 = math.floor(px), math.floor(py)
    x1, y1 = math.ceil(P.page(ax + aw, ay + ah)[0]), math.ceil(P.page(ax + aw, ay + ah)[1])
    w, h = x1 - x0, y1 - y0
    pics = []
    for sheet in sheets:
        piece = sheet.crop((sx, sy, sx + aw, sy + ah))
        layer = Image.new("RGBA", (w * SS, h * SS), (0, 0, 0, 0))
        big = piece.resize((round(aw * P.SX * SS), round(ah * P.SY * SS)), Image.LANCZOS)
        layer.alpha_composite(big, (round((px - x0) * SS), round((py - y0) * SS)))
        im = crop(bg, x0, y0, w, h)
        im.alpha_composite(layer.resize((w, h), Image.LANCZOS))
        pics.append(im)
    return pics[0], pics[1], (x0, y0, w, h)


# ------------------------------------------------------------------------------------------------ strips
def fader_strip(bg, fx):
    x0 = round(P.page(fx - P.FADER_W / 2, 0)[0])
    y0 = round(P.page(0, P.FADER_TOP)[1])
    w = round(P.FADER_W * P.SX)
    h = round((P.FADER_BOTTOM - P.FADER_TOP) * P.SY)
    cap = art("SLIDER CAP one.png")
    cap = cap.resize((w, round(cap.size[1] * w / cap.size[0])), Image.LANCZOS)
    under = crop(bg, x0, y0, w, h)
    frames = min(101, MAX_STRIP // h)
    strip = Image.new("RGB", (w, h * frames))
    for k in range(frames):
        v = k / (frames - 1)
        f = under.copy()
        f.alpha_composite(cap, (0, round((h - cap.size[1]) * (1 - v))))
        strip.paste(f.convert("RGB"), (0, k * h))
    return strip, frames, (x0, y0, w, h)


def knob_strip(bg, name, centre, size=None):
    pic = art(name)
    side = max(pic.size)
    sq = Image.new("RGBA", (side, side), (0, 0, 0, 0))
    sq.alpha_composite(pic, ((side - pic.size[0]) // 2, (side - pic.size[1]) // 2))
    s = round((size or side) * P.S)
    cx, cy = P.page(*centre)
    x0, y0 = round(cx - s / 2), round(cy - s / 2)
    under = crop(bg, x0, y0, s, s)
    big = sq.resize((s * SS, s * SS), Image.LANCZOS)
    strip = Image.new("RGB", (s, s * KNOB_FRAMES))
    for k in range(KNOB_FRAMES):
        ang = -135 + 270 * k / (KNOB_FRAMES - 1)
        f = under.copy()
        f.alpha_composite(big.rotate(-ang, resample=Image.BICUBIC).resize((s, s), Image.LANCZOS))
        strip.paste(f.convert("RGB"), (0, k * s))
    return strip, (x0, y0, s, s)


def bar_strip(w, h, frames=101):
    """a value row's bar: a dark track filling white from the left, a red edge at the value"""
    strip = Image.new("RGB", (w, h * frames), (0, 0, 0))
    for k in range(frames):
        v = k / (frames - 1)
        f = Image.new("RGBA", (w * SS, h * SS), (0, 0, 0, 255))
        d = ImageDraw.Draw(f)
        ty0, ty1 = h * SS * 0.3, h * SS * 0.7
        d.rounded_rectangle([0, ty0, w * SS - 1, ty1], radius=int((ty1 - ty0) / 2), fill=(46, 46, 48, 255))
        fx = round((w * SS - 1) * v)
        if fx > 2:
            d.rounded_rectangle([0, ty0, fx, ty1], radius=int((ty1 - ty0) / 2), fill=(214, 214, 214, 255))
        d.rounded_rectangle([max(0, fx - 3 * SS), 0, min(w * SS - 1, fx + 3 * SS), h * SS - 1], radius=2 * SS,
                            fill=(226, 32, 28, 255))
        strip.paste(f.resize((w, h), Image.LANCZOS).convert("RGB"), (0, k * h))
    return strip, frames


# ------------------------------------------------------------------------------------------------ TUI.json pieces
def bounds(x, y, w, h):
    return "%d %d %d %d" % (round(x), round(y), round(w), round(h))


def sub(name, typ, data, b, visible="Always"):
    return {"version": 2, "componentData": {"version": 1, "name": name, "type": typ, "data": data},
            "handle remapping": {"version": 1, "map": []},
            "bounds": {"version": 2, "acceptsHWFocus": "No", "showWhenDataModelInvalid": "Show", "whenVisible": visible,
                       "boundsType": "Absolute", "bounds": b, "additionalInvalidatingHandles": []}}


def focus(w, h):
    return sub("Focus", "Focus", {"version": 1, "backgroundColour": "00000000", "outlineColour": "00000000",
                                  "backgroundInset": 2.0, "outlineThickness": 0.0}, bounds(0, 0, w, h), "WhenFocussed")


def label(name, kind, b, size, colour, just):
    return sub(name, "Label", {"version": 1, "textStyle": {
        "version": 1, "font": {"version": 1, "name": "Titillium Web", "style": "SemiBold", "height": float(size)},
        "colour": colour, "justification": just, "case": "Original"}, "type": kind, "handleName": "Data"}, b)


def action(on, handler, extra=""):
    return {"version": 2, "onAction": on, "handler": handler, "handleName": "" if handler == "Show Overlay" else "Data",
            "additionalData": extra, "handle remapping": {"version": 1, "map": []}}


KNOB_ACTIONS = [action("Mouse Down", "Q-Link"), action("Double Click", "Show Overlay", "knob overlay"),
                action("Enter Pressed", "Show Overlay", "knob overlay")]
BUTTON_ACTIONS = [action("Mouse Down", "Q-Link"), action("Enter Pressed", "Toggle Switch")]


def definition(key, actions, parts):
    return {"key": key, "value": {
        "version": 4, "actions": actions,
        "backgroundData": {"version": 1, "focussed": {"version": 1, "colour": "0", "image": ""},
                           "unfocussed": {"version": 1, "colour": "0", "image": ""}},
        "ignoreMousePresses": False, "disableCoarseDataWheel": False, "repeats": 1, "hideQLinkBounds": True,
        "componentsData": parts}}


def filmstrip(name, img, frames, w, h, drag="Vertical", x=0, y=0):
    return sub(name, "Knob", {"version": 5, "knobType": "FilmStrip", "filmStrip": img, "numFrames": frames,
                              "invert": False, "dragOrientation": drag, "handleName": "Data"}, bounds(x, y, w, h))


# ------------------------------------------------------------------------------------------------ main
def main(skin):
    tui_path = os.path.join(skin, "TUI.json")
    tui = json.load(open(tui_path))
    cdefs = tui["pageData"]["componentDefinitions"]
    defs = cdefs["localComponentDefinitions"]
    by_key = {e["key"]: e for e in defs}
    params = json.load(open(os.path.join(P.ROOT, "params.json")))["params"]
    index = {p["key"]: i for i, p in enumerate(params)}
    page = by_key["LXRXXL|LXRXXL"]["value"]["componentsData"]
    bg = draw_background()
    new_defs = []
    used = {"sh_bg_0.png"}

    def placed(key):
        tag = "Parameter %d" % index[key]
        comps = [c for c in page if any(m["value"] == tag for m in c["handle remapping"]["map"])]
        if not comps:
            raise SystemExit("panel_art: no control for %s" % key)
        return comps

    def save(img, name):
        img.convert("RGB").save(os.path.join(skin, name))
        used.add(name)

    def put(key, dkey, rect):
        for c in placed(key):
            c["componentData"]["type"] = dkey
            c["bounds"]["bounds"] = bounds(*rect)

    # the keys
    sheets = (art("BUTTONS OFF ALL.png"), art("BUTTONS ON ALL.png"))
    for key, _ in __import__("gen_params").BUTTONS:   # (the mod's keys follow below)
        off, on, (x, y, w, h) = key_pictures(bg, key, sheets)
        save(off, "lxr_key_%s_off.png" % key)
        save(on, "lxr_key_%s_on.png" % key)
        dkey = "lxrKey_%s" % key
        new_defs.append(definition(dkey, BUTTON_ACTIONS, [focus(w, h), sub("Button", "Button", {
            "version": 2, "onImage": "lxr_key_%s_on.png" % key, "offImage": "lxr_key_%s_off.png" % key, "buttonId": 1,
            "numButtonsInGroup": 1, "handleName": "Data", "gestureBehaviour": "Instant"}, bounds(0, 0, w, h))]))
        put(key, dkey, (x, y, w, h))
    # the faders
    for i, fx in enumerate(P.FADERS):
        strip, frames, (x, y, w, h) = fader_strip(bg, fx)
        name = "lxr_fader%d.png" % (i + 1)
        save(strip, name)
        dkey = "lxrFader%d" % (i + 1)
        new_defs.append(definition(dkey, KNOB_ACTIONS, [focus(w, h), filmstrip("Slider", name, frames, w, h)]))
        put("fader%d" % (i + 1), dkey, (x, y, w, h))
    # the pots, their value in the white centre
    for i, c in enumerate(P.POTS):
        strip, (x, y, s, _) = knob_strip(bg, "PARAMETER DATA KNOB.png", c)
        name = "lxr_pot%d.png" % (i + 1)
        save(strip, name)
        disc = P.POT_DISC * P.S
        dkey = "lxrPot%d" % (i + 1)
        new_defs.append(definition(dkey, KNOB_ACTIONS, [
            focus(s, s), filmstrip("Knob", name, KNOB_FRAMES - 1, s, s),
            label("Value", "Value", bounds((s - disc) / 2, (s - disc) / 2, disc, disc), 17, "ff141414",
                  "horizontallyCentred verticallyCentred")]))
        put("pot%d" % (i + 1), dkey, (x, y, s, s))
    # the mod: its two keys and amount knobs (the amount printed in the knob's coloured centre)
    for key, m in list(P.MOD.items()) + [("reset", P.RESET_KEY)]:
        off, on, (x, y, w, h) = mod_key_pictures(bg, m)
        save(off, "lxr_key_%s_off.png" % key)
        save(on, "lxr_key_%s_on.png" % key)
        dkey = "lxrKey_%s" % key
        new_defs.append(definition(dkey, BUTTON_ACTIONS, [focus(w, h), sub("Button", "Button", {
            "version": 2, "onImage": "lxr_key_%s_on.png" % key, "offImage": "lxr_key_%s_off.png" % key, "buttonId": 1,
            "numButtonsInGroup": 1, "handleName": "Data", "gestureBehaviour": "Instant"}, bounds(0, 0, w, h))]))
        put(key, dkey, (x, y, w, h))
        if key == "reset":
            continue
        strip, (x, y, s, _) = knob_strip(bg, m["knob_img"], m["knob"], m.get("knob_size"))
        name = "lxr_%s.png" % m["amt"]
        save(strip, name)
        disc = P.MOD_KNOB_DISC * P.S
        dkey = "lxrKnob_%s" % m["amt"]
        new_defs.append(definition(dkey, KNOB_ACTIONS, [
            focus(s, s), filmstrip("Knob", name, KNOB_FRAMES - 1, s, s),
            label("Value", "Value", bounds((s - disc) / 2 - 4, (s - disc) / 2, disc + 8, disc), 11, "fff2f2f2",
                  "horizontallyCentred verticallyCentred")]))
        put(m["amt"], dkey, (x, y, s, s))
    # the data encoder
    strip, (x, y, s, _) = knob_strip(bg, "Main KNOB.png", P.DATA)
    save(strip, "lxr_data.png")
    new_defs.append(definition("lxrData", KNOB_ACTIONS, [focus(s, s), filmstrip("Knob", "lxr_data.png", KNOB_FRAMES - 1, s, s)]))
    put("data", "lxrData", (x, y, s, s))
    # the value rows: name left, value right, the bar under them
    rx, ry, rw, rh = P.row_box(0)
    strip, frames = bar_strip(rw, P.BAR_H)
    save(strip, "lxr_bar.png")
    text_h = rh - P.BAR_H - 6
    new_defs.append(definition("lxrRow", KNOB_ACTIONS, [
        focus(rw, rh),
        label("Name", "Name", bounds(0, 0, rw * 0.55, text_h), 25, ROW_INK, "left verticallyCentred"),
        label("Value", "Value", bounds(rw * 0.3, 0, rw * 0.7, text_h), 25, ROW_INK, "right verticallyCentred"),
        filmstrip("Slider", "lxr_bar.png", frames, rw, P.BAR_H, "Horizontal", 0, text_h + 4)]))
    for i in range(8):
        put("slot%d" % (i + 1), "lxrRow", P.row_box(i))
    # the display: the panel's window, two lines of MPC's live text
    lx, ly, lw, lh = P.LCD
    x0, y0 = P.page(lx, ly)
    w, h = lw * P.SX, lh * P.SY
    line_h = (h - 8) / 2
    new_defs.append(definition("lxrLcd", [], [label("Value", "Value", bounds(10, 0, w - 20, line_h), 24, LCD_INK,
                                                     "horizontallyCentred verticallyCentred")]))
    for n in (1, 2):
        put("lcd%d" % n, "lxrLcd", (x0, y0 + 4 + (n - 1) * line_h, w, line_h))

    keep = {"LXRXXL|LXRXXL"}
    cdefs["localComponentDefinitions"] = [e for e in defs if e["key"] in keep] + new_defs
    page[0]["componentData"]["data"]["image"] = "sh_bg_0.png"
    save(bg, "sh_bg_0.png")
    json.dump(tui, open(tui_path, "w"), indent=1)
    for f in os.listdir(skin):
        if f.endswith(".png") and f not in used:
            os.remove(os.path.join(skin, f))
    print("panel_art: %d keys, 7 faders, 4 pots, the encoder, 8 value rows" % len(P.KEYS))


if __name__ == "__main__":
    main(sys.argv[1])

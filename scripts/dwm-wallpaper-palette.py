#!/usr/bin/env python3
"""
dwm-wallpaper-palette.py
────────────────────────
Extracts a palette from a wallpaper with Pillow (median-cut, population
weighted) and rewrites the [theme.dynamic] table of ~/.config/dwm/themes.toml,
then sends SIGUSR1 to *this user's* dwm if the dynamic theme is active.

Only the user's own config file is written. Files under the managed install
tree (~/.local/share/dwm) are never modified: the desktop updater fingerprints
them and would report drift.

Requires Pillow. Without it the script prints a notice and exits 0.
"""

import colorsys
import fcntl
import os
import re
import shutil
import signal
import subprocess
import sys
import tempfile

try:
    from PIL import Image
except ImportError:
    print("dwm-wallpaper-palette: Pillow not installed. Skipping dynamic palette extraction.",
          file=sys.stderr)
    sys.exit(0)

# Pillow < 9.1 compatibility
_BILINEAR = getattr(getattr(Image, "Resampling", Image), "BILINEAR")
_MEDIANCUT = getattr(getattr(Image, "Quantize", Image), "MEDIANCUT")


# ── Colour maths ──────────────────────────────────────────────────────────────

def hex_color(rgb):
    return "#{:02x}{:02x}{:02x}".format(*(max(0, min(255, int(round(c)))) for c in rgb))


def _lin(channel):
    c = channel / 255.0
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def rel_luminance(rgb):
    r, g, b = (_lin(c) for c in rgb)
    return 0.2126 * r + 0.7152 * g + 0.0722 * b


def contrast(a, b):
    la, lb = rel_luminance(a), rel_luminance(b)
    hi, lo = max(la, lb), min(la, lb)
    return (hi + 0.05) / (lo + 0.05)


def to_hls(rgb):
    return colorsys.rgb_to_hls(*(c / 255.0 for c in rgb))


def from_hls(h, l, s):
    r, g, b = colorsys.hls_to_rgb(h % 1.0, max(0.0, min(1.0, l)), max(0.0, min(1.0, s)))
    return (round(r * 255), round(g * 255), round(b * 255))


def shift_lightness(rgb, delta):
    h, l, s = to_hls(rgb)
    return from_hls(h, l + delta, s)


def ensure_contrast(rgb, bg, target):
    """Raise lightness (hue/saturation kept) until contrast against bg >= target."""
    h, l, s = to_hls(rgb)
    candidate = rgb
    while contrast(candidate, bg) < target and l < 0.98:
        l += 0.02
        candidate = from_hls(h, l, s)
    return candidate


def saturation(rgb):
    return to_hls(rgb)[2]


# ── Extraction ────────────────────────────────────────────────────────────────

def dominant_colors(image_path, num_colors=16):
    """Return [(rgb, share)] sorted by share descending, using real pixel counts."""
    with Image.open(image_path) as source:
        img = source.convert("RGB")
    img.thumbnail((200, 200), _BILINEAR)
    quantized = img.quantize(colors=num_colors, method=_MEDIANCUT)
    palette = quantized.getpalette() or []
    counted = quantized.getcolors(maxcolors=256) or []
    total = sum(count for count, _ in counted) or 1
    colors = []
    for count, index in counted:
        base = index * 3
        if base + 2 >= len(palette):
            continue
        colors.append(((palette[base], palette[base + 1], palette[base + 2]), count / total))
    colors.sort(key=lambda item: item[1], reverse=True)
    return colors


def extract_palette(image_path):
    colors = dominant_colors(image_path)
    if not colors:
        raise ValueError("no colours extracted")

    # Background: darkest colour that covers a meaningful share, forced dark but not pure black
    meaningful = [c for c in colors if c[1] >= 0.02] or colors
    darkest = min(meaningful, key=lambda item: rel_luminance(item[0]))[0]
    h, l, s = to_hls(darkest)
    bg = from_hls(h, min(max(l, 0.05), 0.10), min(s, 0.45))

    # Accent: most saturated, well-represented colour; lightness raised to reach 4.5:1
    pool = [c for c in colors if c[1] >= 0.01 and saturation(c[0]) >= 0.25 and 0.15 <= to_hls(c[0])[1] <= 0.9]
    if pool:
        base = max(pool, key=lambda item: saturation(item[0]) * (item[1] ** 0.5))[0]
        ah, al, as_ = to_hls(base)
        accent = from_hls(ah, max(al, 0.55), max(as_, 0.45))
    else:
        accent = from_hls(210 / 360.0, 0.60, 0.50)
    accent = ensure_contrast(accent, bg, 4.5)

    # Foreground: tinted near-white, >= 7:1
    lightest = max(colors, key=lambda item: rel_luminance(item[0]))[0]
    fh, _, fs = to_hls(lightest)
    fg = ensure_contrast(from_hls(fh, 0.90, min(fs, 0.12)), bg, 7.0)

    border_norm = shift_lightness(bg, 0.12)
    selected_bg = shift_lightness(bg, 0.07)

    ansi_hues = [0, 120, 50, 215, 300, 180]  # R G Y B M C
    normal = [ensure_contrast(from_hls(hue / 360.0, 0.62, 0.60), bg, 4.5) for hue in ansi_hues]
    bright = [ensure_contrast(from_hls(hue / 360.0, 0.76, 0.65), bg, 7.0) for hue in ansi_hues]

    c0 = shift_lightness(bg, 0.04)
    c7 = ensure_contrast((216, 222, 233), bg, 7.0)
    c8 = ensure_contrast(shift_lightness(bg, 0.22), bg, 3.0)
    c15 = ensure_contrast((236, 239, 244), bg, 10.0)
    full_ansi = [c0, *normal, c7, c8, *bright, c15]

    return {
        "normbgcolor": hex_color(bg),
        "normfgcolor": hex_color(fg),
        "normbordercolor": hex_color(border_norm),
        "selbgcolor": hex_color(selected_bg),
        "selfgcolor": hex_color(ensure_contrast((236, 239, 244), selected_bg, 7.0)),
        "selbordercolor": hex_color(accent),
        "term_bg": hex_color(bg),
        "term_fg": hex_color(fg),
        "term_cursor": hex_color(accent),
        "colors": [hex_color(c) for c in full_ansi],
    }


# ── themes.toml handling ──────────────────────────────────────────────────────

_DYNAMIC_HEADER = re.compile(r"^\s*\[theme\.dynamic\]\s*(#.*)?$")
_ANY_TABLE = re.compile(r"^\s*\[")
_SECTION_COMMENT = "# ── Dynamic (Extracted from Wallpaper) ────────────────────────────────────────"


def dynamic_block_lines(palette):
    lines = [
        _SECTION_COMMENT,
        "[theme.dynamic]",
        f'normfgcolor     = "{palette["normfgcolor"]}"',
        f'normbgcolor     = "{palette["normbgcolor"]}"',
        f'normbordercolor = "{palette["normbordercolor"]}"',
        f'selfgcolor      = "{palette["selfgcolor"]}"',
        f'selbgcolor      = "{palette["selbgcolor"]}"',
        f'selbordercolor  = "{palette["selbordercolor"]}"',
        "",
        f'term_bg         = "{palette["term_bg"]}"',
        f'term_fg         = "{palette["term_fg"]}"',
        f'term_cursor     = "{palette["term_cursor"]}"',
    ]
    lines += [f'term_color{i:<4}= "{value}"' for i, value in enumerate(palette["colors"])]
    lines += ["", "dark_mode       = true"]
    return lines


def replace_dynamic_section(content, palette):
    """Replace only the [theme.dynamic] table; every other table is left untouched."""
    lines = content.splitlines()
    block = dynamic_block_lines(palette)
    header = next((i for i, line in enumerate(lines) if _DYNAMIC_HEADER.match(line)), None)

    if header is None:
        while lines and not lines[-1].strip():
            lines.pop()
        return "\n".join(lines + ([""] if lines else []) + block) + "\n"

    start = header
    if start > 0 and lines[start - 1].lstrip().startswith("# ── Dynamic"):
        start -= 1
    end = next((i for i in range(header + 1, len(lines)) if _ANY_TABLE.match(lines[i])), len(lines))
    # Comments/blank lines directly before the next table belong to that table
    while end > header + 1 and (not lines[end - 1].strip() or lines[end - 1].lstrip().startswith("#")):
        end -= 1
    rest = lines[end:]
    if rest and rest[0].strip():
        block = block + [""]
    return "\n".join(lines[:start] + block + rest).rstrip("\n") + "\n"


def update_themes_toml(palette, toml_path):
    target = os.path.realpath(toml_path)  # keep dotfile-manager symlinks intact
    directory = os.path.dirname(target)
    lock_path = os.path.join(directory, ".themes.toml.lock")
    with open(lock_path, "a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        with open(target, "r", encoding="utf-8") as stream:
            content = stream.read()
        new_content = replace_dynamic_section(content, palette)
        if new_content == content:
            return False
        mode = os.stat(target).st_mode & 0o7777
        descriptor, temporary = tempfile.mkstemp(prefix=".themes.toml.", dir=directory)
        try:
            with os.fdopen(descriptor, "w", encoding="utf-8") as stream:
                stream.write(new_content)
                stream.flush()
                os.fsync(stream.fileno())
            os.chmod(temporary, mode)
            os.replace(temporary, target)
        except BaseException:
            try:
                os.unlink(temporary)
            except OSError:
                pass
            raise
    return True


def is_dynamic_active(toml_path):
    try:
        with open(toml_path, "r", encoding="utf-8") as stream:
            content = stream.read()
    except OSError:
        return False
    match = re.search(r'^\s*\[active\]\s*$(.*?)(?=^\s*\[|\Z)', content, re.M | re.S)
    if not match:
        return False
    value = re.search(r'^\s*theme\s*=\s*["\']([^"\']+)["\']', match.group(1), re.M)
    return bool(value and value.group(1).strip() == "dynamic")


def reload_dwm():
    """SIGUSR1 to the current user's dwm only."""
    try:
        output = subprocess.check_output(["pgrep", "-x", "-u", str(os.getuid()), "dwm"], text=True)
        for pid in output.split():
            os.kill(int(pid), signal.SIGUSR1)
    except subprocess.CalledProcessError:
        pass  # dwm not running for this user
    except (OSError, subprocess.SubprocessError, ValueError) as error:
        print(f"dwm-wallpaper-palette: could not signal dwm: {error}", file=sys.stderr)


def seed_user_config(toml_path):
    """Create the user's themes.toml from the installed default if it does not exist."""
    if os.path.exists(toml_path):
        return True
    data_home = os.environ.get("XDG_DATA_HOME", os.path.expanduser("~/.local/share"))
    default = os.path.join(data_home, "dwm", "config", "themes.toml")
    if not os.path.isfile(default):
        return False
    os.makedirs(os.path.dirname(toml_path), exist_ok=True)
    shutil.copyfile(default, toml_path)  # read-only access to the managed tree
    return True


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <path_to_wallpaper_image>", file=sys.stderr)
        return 1
    wallpaper = sys.argv[1]
    if not os.path.isfile(wallpaper):
        print(f"dwm-wallpaper-palette: not a file: {wallpaper}", file=sys.stderr)
        return 1

    config_home = os.environ.get("XDG_CONFIG_HOME", os.path.expanduser("~/.config"))
    toml_path = os.path.join(config_home, "dwm", "themes.toml")
    if not seed_user_config(toml_path):
        print("dwm-wallpaper-palette: no themes.toml found", file=sys.stderr)
        return 1

    try:
        palette = extract_palette(wallpaper)
    except (OSError, ValueError, Image.DecompressionBombError) as error:
        print(f"dwm-wallpaper-palette: cannot read image: {error}", file=sys.stderr)
        return 1

    updated = update_themes_toml(palette, toml_path)
    if updated and is_dynamic_active(toml_path):
        reload_dwm()
    return 0


if __name__ == "__main__":
    sys.exit(main())

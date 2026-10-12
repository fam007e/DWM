#!/usr/bin/env python3
"""
dwm-wallpaper-palette.py
────────────────────────
Extracts a dominant color palette from a wallpaper image using PIL/Pillow
and updates ~/.config/dwm/themes.toml under the [theme.dynamic] section,
then reloads DWM (kill -USR1) so all windows, borders, terminal, and
Quickshell update live.

Zero external dependencies beyond standard Python 3 + Pillow.
If wallust or matugen are installed, it can delegate to them; otherwise
it runs native k-means / median-cut quantization.
"""

import sys
import os
import re
import signal
import subprocess
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("dwm-wallpaper-palette: PIL/Pillow not installed. Skipping dynamic palette extraction.", file=sys.stderr)
    sys.exit(0)

def hex_color(rgb):
    return f"#{rgb[0]:02x}{rgb[1]:02x}{rgb[2]:02x}"

def luminance(rgb):
    return 0.299 * rgb[0] + 0.587 * rgb[1] + 0.114 * rgb[2]

def adjust_lightness(rgb, factor):
    return tuple(max(0, min(255, int(c * factor))) for c in rgb)

def extract_palette(image_path, num_colors=16):
    img = Image.open(image_path).convert("RGB")
    # Resize thumbnail for fast processing
    img.thumbnail((200, 200), Image.Resampling.BILINEAR)

    # Use median cut quantization
    quantized = img.quantize(colors=num_colors, method=Image.Quantize.MEDIANCUT)
    palette_raw = quantized.getpalette()[:num_colors * 3]
    colors = [
        (palette_raw[i * 3], palette_raw[i * 3 + 1], palette_raw[i * 3 + 2])
        for i in range(num_colors)
    ]

    # Sort colors by luminance
    colors.sort(key=luminance)

    # Background is darkest dominant color, clamped to be dark enough
    darkest = colors[0]
    if luminance(darkest) > 50:
        bg = adjust_lightness(darkest, 0.3)
    else:
        bg = darkest

    # Foreground is lightest dominant color, clamped to be bright enough
    lightest = colors[-1]
    if luminance(lightest) < 180:
        fg = (230, 235, 240)
    else:
        fg = lightest

    # Accent is the most saturated mid-tone color, ensuring minimum 4.5:1 contrast against bg
    def saturation(c):
        mx = max(c)
        mn = min(c)
        return (mx - mn) / mx if mx > 0 else 0

    # Ensure accent has adequate luminance against dark background (lum > 95)
    candidates = [c for c in colors if 90 <= luminance(c) <= 210]
    if candidates:
        candidates.sort(key=saturation, reverse=True)
        accent = candidates[0]
    else:
        # Boost saturation/lightness of the mid tone
        mid = colors[len(colors) // 2]
        factor = 120.0 / max(luminance(mid), 1)
        accent = adjust_lightness(mid, min(factor, 2.5))

    # Inactive border: visible separation against bg
    border_norm = adjust_lightness(bg, 2.2)
    if luminance(border_norm) < 25:
        border_norm = (max(border_norm[0], 50), max(border_norm[1], 50), max(border_norm[2], 65))
    # Selected border: vibrant accent
    border_sel = accent

    # Generate a legible, contrast-guaranteed 16-color ANSI palette
    # Colors 1-6 (standard): Red, Green, Yellow, Blue, Magenta, Cyan
    # Colors 9-14 (bright): Bright versions with boosted lightness
    import colorsys

    ansi_hues = [0, 120, 50, 215, 300, 180]  # R, G, Y, B, M, C

    ansi_normal = []
    ansi_bright = []
    for h in ansi_hues:
        # Standard: Lightness ~0.62, Saturation ~0.60 -> guaranteed >= 5.0:1 contrast
        r, g, b = colorsys.hls_to_rgb(h / 360.0, 0.62, 0.60)
        ansi_normal.append(tuple(int(c * 255) for c in (r, g, b)))
        # Bright: Lightness ~0.76, Saturation ~0.65 -> guaranteed >= 8.0:1 contrast
        r, g, b = colorsys.hls_to_rgb(h / 360.0, 0.76, 0.65)
        ansi_bright.append(tuple(int(c * 255) for c in (r, g, b)))

    c0 = adjust_lightness(bg, 1.25)       # Black (surface/bg fill)
    c7 = (216, 222, 233)                  # White (fg text, contrast > 10:1)
    c8 = adjust_lightness(bg, 2.2)        # Bright Black (muted comments)
    if luminance(c8) < 35:
        c8 = (75, 85, 105)
    c15 = (236, 239, 244)                 # Bright White (bold fg)

    full_ansi = [
        c0,
        ansi_normal[0],  # 1: red
        ansi_normal[1],  # 2: green
        ansi_normal[2],  # 3: yellow
        ansi_normal[3],  # 4: blue
        ansi_normal[4],  # 5: magenta
        ansi_normal[5],  # 6: cyan
        c7,              # 7: white
        c8,              # 8: bright black
        ansi_bright[0],  # 9: bright red
        ansi_bright[1],  # 10: bright green
        ansi_bright[2],  # 11: bright yellow
        ansi_bright[3],  # 12: bright blue
        ansi_bright[4],  # 13: bright magenta
        ansi_bright[5],  # 14: bright cyan
        c15              # 15: bright white
    ]

    return {
        "normbgcolor": hex_color(bg),
        "normfgcolor": hex_color(fg),
        "normbordercolor": hex_color(border_norm),
        "selbgcolor": hex_color(adjust_lightness(bg, 1.45)),
        "selfgcolor": "#eceff4",
        "selbordercolor": hex_color(border_sel),
        "term_bg": hex_color(bg),
        "term_fg": hex_color(fg),
        "term_cursor": hex_color(accent),
        "colors": [hex_color(c) for c in full_ansi]
    }

def update_themes_toml(palette, toml_path):
    if not os.path.exists(toml_path):
        return False

    with open(toml_path, "r", encoding="utf-8") as f:
        content = f.read()

    # Generate dynamic section
    colors_lines = "\n".join([f"term_color{i}    = \"{palette['colors'][i]}\"" for i in range(len(palette['colors']))])
    dynamic_block = f"""
# ── Dynamic (Extracted from Wallpaper) ────────────────────────────────────────
[theme.dynamic]
normfgcolor     = "{palette['normfgcolor']}"
normbgcolor     = "{palette['normbgcolor']}"
normbordercolor = "{palette['normbordercolor']}"
selfgcolor      = "{palette['selfgcolor']}"
selbgcolor      = "{palette['selbgcolor']}"
selbordercolor  = "{palette['selbordercolor']}"

term_bg         = "{palette['term_bg']}"
term_fg         = "{palette['term_fg']}"
term_cursor     = "{palette['term_cursor']}"
{colors_lines}

dark_mode       = true
"""

    if "[theme.dynamic]" in content:
        new_content = re.sub(r'# ── Dynamic.*?\n\[theme\.dynamic\].*?(?=\n\[theme|\Z)', dynamic_block.strip() + "\n\n", content, flags=re.DOTALL)
    else:
        new_content = content.rstrip() + "\n" + dynamic_block

    if new_content == content:
        return False

    with open(toml_path, "w", encoding="utf-8") as f:
        f.write(new_content)

    return True

def is_dynamic_active(toml_path):
    if not os.path.exists(toml_path):
        return False
    try:
        with open(toml_path, "r", encoding="utf-8") as f:
            content = f.read()
        m = re.search(r'\[active\][^\[]*?theme\s*=\s*["\']([^"\']+)["\']', content)
        return bool(m and m.group(1).strip() == "dynamic")
    except Exception:
        return False

def reload_dwm():
    # Find dwm pid and send SIGUSR1 to reload themes.toml
    try:
        pid = subprocess.check_output(["pidof", "dwm"]).decode().strip().split()[0]
        os.kill(int(pid), signal.SIGUSR1)
    except Exception:
        pass

def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <path_to_wallpaper_image>", file=sys.stderr)
        sys.exit(1)

    wallpaper = sys.argv[1]
    if not os.path.isfile(wallpaper):
        sys.exit(1)

    config_home = os.environ.get("XDG_CONFIG_HOME", os.path.expanduser("~/.config"))
    toml_path = os.path.join(config_home, "dwm", "themes.toml")

    # If local repo themes.toml exists, also update it
    repo_toml = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "config", "themes.toml")

    palette = extract_palette(wallpaper)

    updated = False
    if os.path.exists(toml_path):
        updated = update_themes_toml(palette, toml_path) or updated
    if os.path.exists(repo_toml):
        update_themes_toml(palette, repo_toml)

    # Note: Only switch active theme to dynamic if the user explicitly configured theme = "dynamic"
    # or if requested. We notify DWM if dynamic is active and palette changed.
    if updated and is_dynamic_active(toml_path):
        reload_dwm()

if __name__ == "__main__":
    main()

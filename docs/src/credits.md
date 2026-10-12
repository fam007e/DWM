# Inspiration & Acknowledgments

This project stands on the shoulders of the open-source X11 and tiling window manager community.

---

## 🌟 Primary Inspiration: Chris Titus Tech (`dwm-titus`)

Special acknowledgment is due to [Chris Titus Tech (CTT)](https://github.com/ChrisTitusTech) and the [dwm-titus](https://github.com/ChrisTitusTech/dwm-titus) project.

### Core Ideas Adopted
- **Quickshell Integration Concept:** The pioneering concept of using Quickshell (a modern Qt Quick/QML shell) as a modern companion to DWM.
- **EWMH Custom Atom Bridge:** Exposing internal DWM monitor topologies, active tags, and client lists to user space via custom root window X11 atoms (`_NET_DESKTOP_NAMES`, `_DWM_MONITOR_DESKTOPS`, `_DWM_FULLSCREEN_MONITORS`).

### Enhancements & Divergence in this Fork
While inspired by Titus's desktop shell vision, this repository represents an independent, hardened rebuild tailored specifically for Arch Linux rolling releases:
1. **Security & Memory Hardening:** Corrected multiple memory safety hazards identified during our security audit, including null pointer dereferences on monitor reallocation (`AUD-01`), integer overflow bounds checks on 64-bit multi-monitor atoms (`AUD-03`, `AUD-05`), and input sanitization on untrusted X11 client titles (`AUD-06`).
2. **Generalized Monitor Topologies:** Replaced the hardcoded vertical coordinate heuristic (`getmonlogicalindex() >= 1080`) with geometric coordinate sorting, ensuring proper behavior across arbitrary multi-monitor topologies.
3. **Atomic Client List Updates:** Replaced $O(N)$ repeated `XChangeProperty` appends with atomic batch replacement (`AUD-04`), preventing event storms against event-driven desktop watchers.
4. **Complete Polybar & Rofi Eradication:** Fully purged legacy Polybar scripts, assets, configs, and dependencies. Replaced rofi dependencies with native Quickshell controls and zero-dependency dialogs (`zenity`, `qalc`).
5. **Native Quickshell Weather Widget:** Developed a native QML weather popup and panel pill powered by `scripts/OpWeatherinfo`, featuring automatic `wttr.in` fallback.
6. **Dynamic Wallpaper Quantization:** Built a custom Python median-cut color extractor (`dwm-wallpaper-palette.py`) integrated with feh and DWM live theme reloading.
7. **Native C Warm Mode:** Integrated XRandR gamma shift directly into the DWM binary, eliminating the need for third-party background daemons.

---

## 🏛️ Upstream Projects & Tooling

- **[suckless.org](https://dwm.suckless.org/):** For creating the original dynamic window manager (`dwm`), the minimalist foundation of keyboard-driven window management on X11.
- **[Quickshell](https://outfoxxed.me/quickshell/):** The flexible, scriptable Qt Quick shell framework created by outfoxxed.
- **[Nord Theme](https://www.nordtheme.com/):** For the arctic color palette inspiration.
- **[Nerd Fonts](https://www.nerdfonts.com/):** For glyph coverage and icon sets.

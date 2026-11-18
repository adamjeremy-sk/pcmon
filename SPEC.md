# pcmon: spec

A small always-visible GTK 4 window that shows the health of this PC at a glance: compact, bars with green / orange / red, an Options popover, and a Close button.

Status: the behaviour below is **decided** (it is what the `pymon` prototype did), except where marked *Proposed*.

## What it shows

| Section | Readings | Source |
|---|---|---|
| CPU | Load %, package temperature (bar vs. critical temp) | `/proc/stat` (busy time between updates); hwmon `coretemp` / `k10temp`, fallback thermal zone |
| Each GPU | Load %, temperatures (edge / junction / memory on AMD), VRAM used / total, fan | sysfs `gpu_busy_percent`, hwmon, `mem_info_vram_*`; `nvidia-smi` for NVIDIA |
| Fans | Every system fan that is spinning, in RPM | hwmon `fan*_input` |
| Memory | RAM used / total, percent | `/proc/meminfo` (used = total - available) |

Behaviour:

- **Every GPU found is shown**, one section each. An integrated GPU has no VRAM and usually no sensors, so it shows "shared with system RAM".
- **Anything that does not exist is not shown.** A missing fan sensor never breaks the window.
- **Fan names** are set in Options (one name box per detected fan) and saved in `settings.ini`. Defaults: fan1 = CPU Tower, fan5 = Front intake, fan6 = Rear exhaust, fan7 = Top case (nct6687 chip, matched to the author's case).
- **Named fans are always shown, even at 0 RPM**, so a stopped case fan stands out. Unnamed system fans are hidden at 0 RPM (empty headers report 0). GPU fans are always shown, since 0 RPM is normal at idle.
- **Colours:** temperatures turn orange at 75% and red at 90% of the sensor's own critical temperature; VRAM, RAM and load turn orange at 70% and red at 90%.
- **Refresh:** every 1 second (`--interval`, minimum 0.1).
- **Options** (saved in `~/.config/pcmon/settings.ini`): Theme (System / Light / Dark, default System), Always on top, opacity slider (30-100%, enabled only while on top), show or hide the "shared with system RAM" line, and fan names. With that line off, an integrated GPU that has nothing else to show is hidden entirely.

## Deliberately left out

- Disk and network usage (decided 2026-11-18).
- Clocks and power draw. Easy to add later from the same sources.
- History graphs. Numbers and bars only.

## Design

- Plain C (GNU C11) with GLib, GTK 4. No C++.
- `src/core` reads the hardware and knows nothing about GTK. `src/ui` is the window. `pcmon-sensors` prints what was detected as JSON.
- Sensors are read on a background thread so a slow driver cannot freeze the window.
- Always on top uses libX11 (`_NET_WM_STATE_ABOVE`), because GTK 4 removed `gtk_window_set_keep_above`.

## Differences from the Python prototype

- Renamed from pymon to pcmon; settings live in `~/.config/pcmon/settings.ini`, with a one-time read of the old pymon file.
- The prototype's migration from an older `settings.json` was dropped.
- **Always on top works on X11 only.** Wayland gives applications no way to ask for it, so the box is disabled there.
- Theme choice and the icon / install script are additions (2026-11-18). The colours are theme-neutral, so dark mode only flips the GTK theme variant.
- Licence: GPL-3.0-or-later, SPDX headers in every source file.

## Known limits

- **Fan sensors depend on drivers.** AMD GPU fans work out of the box. Motherboard fans only appear if the kernel has a driver for the board's chip (usually `nct6683`, `nct6775` or `it87`).
- **Intel iGPU temperature** is normally not exposed separately; it shares the CPU package sensor. **Intel iGPU load** needs root (`intel_gpu_top`), so it is not shown.
- **CPU load is overall**, one number across all cores, measured between updates.
- **NVIDIA support is untested** (no NVIDIA card available); it reads `nvidia-smi` when present.

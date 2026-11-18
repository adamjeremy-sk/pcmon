# Architecture

```
src/core/   sensor library, GLib only (no GTK)  -> libpcmon_core.a, pcmon-sensors
src/ui/     GTK 4 window                        -> pcmon
data/       icon (pcmon.svg, pcmon.png 512 px) and the .desktop entry
tests/      GLib g_test programs on a fake /sys + /proc tree
tools/      install.sh, release.sh, bundle_install.sh, check_sources.sh, check_format.sh
```

## core

| File | Job |
|---|---|
| `types` | `PcSnapshot` and its parts (`PcCpu`, `PcGpu`, `PcTemp`, `PcFan`, `PcRam`), `PcOpt` (value or "not available") |
| `fsread` | read and parse small sysfs files; natural-ordered directory globs |
| `hwmon` | temperatures and fans of one hwmon directory |
| `cpu`, `ram` | `/proc/stat`, `/proc/cpuinfo`, `/proc/meminfo`, CPU package temperature |
| `gpu`, `nvidia`, `proc_run` | every GPU from `/sys/class/drm`; `lspci` and `nvidia-smi` helpers with a timeout |
| `sensors` | `pc_sensors_collect()` builds one `PcSnapshot`; the only entry point the UI needs |
| `json` | snapshot as JSON (used by `pcmon-sensors` and the tests) |
| `settings`, `format` | settings file; display names and number formatting |

Everything takes a root path (`/` in production), so tests build a fake machine in a temp folder. External helpers (`lspci`, `nvidia-smi`) only run when the root is `/`.

## ui

`main` parses options and starts the `GtkApplication`. `window` owns the settings and sections and redraws on every snapshot. `poller` runs `pc_sensors_collect` on a worker thread and hands each snapshot to the main thread with `g_idle_add`. `options` is the popover. `section`/`row`/`label`/`style` are small widget helpers. `keep_above` sends the X11 request.

Thread rules: only the poller thread touches `PcSensors`; only the main thread touches widgets. A snapshot is owned by whoever holds it. After `pc_poller_stop` the callback is never called again.

## Conventions

- Every `.c`, `.h`, `.sh` starts with an SPDX line, the copyright line and a one-line purpose comment (`tools/check_sources.sh` enforces it).
- clang-format 18 (`.clang-format`); `tools/check_format.sh` checks it.
- Warnings are errors (`-Wall -Wextra -Werror`).
- Files over 300 lines are flagged, over 500 fail (tests: 500 / 800).

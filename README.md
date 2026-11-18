# pcmon

A small GTK 4 window showing CPU load and temperature, every GPU's load / temperatures / VRAM / fan, system fan speeds, and RAM usage. Written in C. See `SPEC.md` for details.

pcmon is a C port of the Python/GTK 3 prototype `pymon`.

## Install

```
tools/install.sh              # builds, tests, installs to ~/.local (menu entry + icon)
tools/install.sh --uninstall  # removes it again (settings are kept)
```

`--prefix /usr/local` with `sudo` installs for all users. It needs the packages listed below; the script tells you if one is missing.

## Make a release zip

```
tools/release.sh
```

Builds in release mode, runs the tests and writes `dist/pcmon-VERSION-linux-ARCH.zip` plus a `.sha256` file. The zip holds the binaries, icon, menu entry, README, LICENSE and an `install.sh` that copies them to `~/.local` (or `--prefix DIR`). The target machine needs the GTK 4 runtime (`sudo apt install libgtk-4-1`) and a GTK/glibc at least as new as the build machine's, so build on the oldest Ubuntu you want to support. The version comes from `project(pcmon VERSION ...)` in `CMakeLists.txt`.

## Build and run

```
sudo apt install cmake gcc pkg-config libgtk-4-dev libx11-dev clang-format
cmake -S . -B build
cmake --build build -j
./build/src/ui/pcmon
```

`libx11-dev` is optional. Without it "Always on top" is disabled. Run the tests with `ctest --test-dir build --output-on-failure`.

## See what it detected

```
./build/src/core/pcmon-sensors
```

Prints every sensor it found as JSON. If something is missing from the window, check here first.

## Options

- `--interval 1.0` seconds between updates
- **Options** button in the window:
  - Theme: System, Light or Dark (Dark and Light switch the GTK theme's dark variant; System leaves the desktop's choice alone)
  - Always on top (X11 only), and opacity (30-100%, only while on top)
  - Show or hide the "shared with system RAM" line (hides the Intel iGPU section when off)
  - Fan names: a box for every detected fan. Name a fan to always show it, even at 0 RPM; clear the name to hide it while stopped.

  Everything is saved to `~/.config/pcmon/settings.ini`, a plain text file you can also edit by hand while pcmon is closed. On first run, if that file does not exist, pcmon reads `~/.config/pymon/settings.ini` once.

  ```
  [window]
  theme = dark
  always_on_top = true
  opacity = 80

  [display]
  show_shared_memory = false

  [fans]
  fan1 = CPU Tower
  fan7 = Top case
  ```

## Motherboard fan speeds (nct6683 chip)

The driver is already part of the Ubuntu kernel, so there is nothing to download. It only has to be switched on.

1. Try loading it:

   ```
   sudo modprobe nct6683
   ./build/src/core/pcmon-sensors      # look for fans under "fans"
   ```

2. If no fans appear, the driver probably declined your board (by default it only starts on Intel-brand boards). Check the reason, then force it on:

   ```
   sudo dmesg | tail -5
   sudo modprobe -r nct6683
   sudo modprobe nct6683 force=1
   ```

3. Once it works, make it load at every boot:

   ```
   echo nct6683 | sudo tee /etc/modules-load.d/nct6683.conf
   echo "options nct6683 force=1" | sudo tee /etc/modprobe.d/nct6683.conf    # only if step 2 needed force=1
   ```

Notes:

- The chip reports plain names (`fan1`, `fan2`...). You name them in Options. Until you do, pcmon uses fan1 = CPU Tower, fan5 = Front intake, fan6 = Rear exhaust, fan7 = Top case.
- The kernel's driver is read-only on these boards, so pcmon shows fan speeds but never changes them.
- With `force=1`, double-check that a reading looks sensible; the driver is only verified on some boards.

## License

GPL-3.0-or-later. See `LICENSE`.

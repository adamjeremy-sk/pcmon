# Handoff

## State

- The C port of the sensor layer, settings and formatting is complete. It matches the Python prototype: both print identical JSON for the same fake machine, and the unit tests pass (also clean under ASan/UBSan).
- The GTK 4 UI (`src/ui`) is written but **has never been compiled**: the environment it was written in had no GTK 4 headers. Expect to fix a few compile errors on first build, and check the window by eye.
- Always on top (X11), the Theme dropdown, the icon/desktop entry and `tools/install.sh` are likewise untried on a real desktop (the settings code and CMake install rules were tested).

## Next steps

0. Release: after step 3 below works, run `tools/release.sh` and test the zip on a clean machine. The packaging steps were tested without GTK; the real `pcmon` binary has not been built yet.

1. `cmake -S . -B build && cmake --build build -j && ctest --test-dir build --output-on-failure`
2. Run `./build/src/ui/pcmon`; compare with the Python prototype.
3. Check: Options popover, fan names persist, opacity, always on top, closing the window exits cleanly.
4. Commit (the author commits by hand).

## Ideas, not decided

- Raster icon sizes (only a scalable SVG is installed).
- "Light" cannot override a desktop that uses a dark GTK theme by name.
- Wayland: no way to stay on top without a compositor protocol; left out.

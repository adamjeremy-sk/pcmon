# Notes for an AI assistant

Read `README.md`, `SPEC.md`, `ARCHITECTURE.md`, `HANDOFF.md` first.

## Working rules

- Chat before big changes; do not jump into coding.
- Plain C (GNU C11) + GLib + GTK 4. No C++.
- Every source file has the SPDX/copyright/purpose header; run `tools/check_sources.sh` and `tools/check_format.sh`.
- The author builds and commits. Do not run `git commit` or `git init` unless asked.

## Decided

- The project is a C port of the Python prototype `pymon` (kept in `~/Projects/pymon`); behaviour in `SPEC.md` is the spec.
- Name `pcmon`; settings `~/.config/pcmon/settings.ini` (one-time fallback to the pymon file).
- GPL-3.0-or-later.
- Always on top via libX11; X11 only.
- The old C++ design (`pcmon` with a sensor tree and graphs) was abandoned.

## Proposed, not yet agreed

- Anything listed under "Ideas" in `HANDOFF.md`.

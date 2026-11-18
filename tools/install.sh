#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
# Builds pcmon and installs it with its menu entry and icon (or removes it again).
set -euo pipefail

prefix="$HOME/.local"
uninstall=0
run_tests=1

usage() {
  cat <<'USAGE'
Usage: tools/install.sh [--prefix DIR] [--skip-tests] [--uninstall]

  --prefix DIR   install under DIR (default: ~/.local, no sudo needed;
                 use --prefix /usr/local and run with sudo for all users)
  --skip-tests   do not run the unit tests before installing
  --uninstall    remove what a previous install put under the prefix
USAGE
}

while [ $# -gt 0 ]; do
  case "$1" in
    --prefix) prefix="${2:?--prefix needs a folder}"; shift 2 ;;
    --skip-tests) run_tests=0; shift ;;
    --uninstall) uninstall=1; shift ;;
    -h | --help) usage; exit 0 ;;
    *) usage >&2; exit 2 ;;
  esac
done

source_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="$source_dir/build"
desktop_file="$prefix/share/applications/io.github.adamjeremy_sk.Pcmon.desktop"
icon_file="$prefix/share/icons/hicolor/scalable/apps/pcmon.svg"

refresh_caches() {
  if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$prefix/share/applications" 2>/dev/null || true
  fi
  if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -q -t "$prefix/share/icons/hicolor" 2>/dev/null || true
  fi
}

if [ "$uninstall" -eq 1 ]; then
  rm -f "$prefix/bin/pcmon" "$prefix/bin/pcmon-sensors" "$desktop_file" "$icon_file"
  refresh_caches
  printf 'Removed pcmon from %s\n' "$prefix"
  printf 'Your settings in ~/.config/pcmon were kept.\n'
  exit 0
fi

missing=()
for tool in cmake gcc pkg-config; do
  command -v "$tool" >/dev/null 2>&1 || missing+=("$tool")
done
pkg-config --exists gtk4 2>/dev/null || missing+=("libgtk-4-dev")
if [ ${#missing[@]} -gt 0 ]; then
  printf 'Missing: %s\n' "${missing[*]}" >&2
  printf 'On Ubuntu: sudo apt install cmake gcc pkg-config libgtk-4-dev libx11-dev\n' >&2
  exit 1
fi
if ! pkg-config --exists x11 2>/dev/null; then
  printf 'Note: libx11-dev not found, so "Always on top" will be disabled.\n'
  printf '      Install it (sudo apt install libx11-dev) and run this again to enable it.\n'
fi

cmake -S "$source_dir" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$prefix"
cmake --build "$build_dir" -j "$(nproc)"
if [ "$run_tests" -eq 1 ]; then
  ctest --test-dir "$build_dir" --output-on-failure
fi
cmake --install "$build_dir"
refresh_caches

printf '\nInstalled pcmon to %s\n' "$prefix"
case ":$PATH:" in
  *":$prefix/bin:"*) ;;
  *) printf 'Note: %s/bin is not on your PATH; log out and in again, or add it.\n' "$prefix" ;;
esac
printf 'Start it from your application menu, or run: pcmon\n'

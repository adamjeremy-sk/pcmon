#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
# Installer shipped inside the release zip: copies the prebuilt files under a prefix.
set -euo pipefail

prefix="$HOME/.local"
uninstall=0

while [ $# -gt 0 ]; do
  case "$1" in
    --prefix) prefix="${2:?--prefix needs a folder}"; shift 2 ;;
    --uninstall) uninstall=1; shift ;;
    -h | --help)
      printf 'Usage: ./install.sh [--prefix DIR] [--uninstall]\nDefault prefix: ~/.local\n'
      exit 0 ;;
    *) printf 'Unknown option: %s\n' "$1" >&2; exit 2 ;;
  esac
done

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

refresh_caches() {
  if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$prefix/share/applications" 2>/dev/null || true
  fi
  if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -q -t "$prefix/share/icons/hicolor" 2>/dev/null || true
  fi
}

if [ "$uninstall" -eq 1 ]; then
  rm -f "$prefix/bin/pcmon" "$prefix/bin/pcmon-sensors" \
    "$prefix/share/applications/io.github.adamjeremy_sk.Pcmon.desktop" \
    "$prefix/share/icons/hicolor/scalable/apps/pcmon.svg" \
    "$prefix/share/icons/hicolor/512x512/apps/pcmon.png"
  refresh_caches
  printf 'Removed pcmon from %s (settings in ~/.config/pcmon were kept).\n' "$prefix"
  exit 0
fi

if command -v ldd >/dev/null 2>&1 && ldd "$here/bin/pcmon" 2>/dev/null | grep -q 'not found'; then
  printf 'pcmon needs libraries that are missing:\n' >&2
  ldd "$here/bin/pcmon" | grep 'not found' >&2
  printf 'On Ubuntu: sudo apt install libgtk-4-1\n' >&2
  exit 1
fi

mkdir -p "$prefix"
cp -a "$here/bin" "$here/share" "$prefix/"
refresh_caches

printf 'Installed pcmon to %s\n' "$prefix"
case ":$PATH:" in
  *":$prefix/bin:"*) ;;
  *) printf 'Note: %s/bin is not on your PATH; log out and in again, or add it.\n' "$prefix" ;;
esac
printf 'Start it from your application menu, or run: pcmon\n'

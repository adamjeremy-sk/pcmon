#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
# Builds pcmon in release mode and packs the binary release as dist/pcmon-VERSION-linux-ARCH.zip.
set -euo pipefail

source_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="$source_dir/build-release"
dist_dir="$source_dir/dist"

version="$(sed -n 's/^project(pcmon VERSION \([0-9][0-9.]*\).*/\1/p' "$source_dir/CMakeLists.txt")"
[ -n "$version" ] || { printf 'Could not read the version from CMakeLists.txt\n' >&2; exit 1; }
name="pcmon-$version-linux-$(uname -m)"
stage="$dist_dir/$name"

for tool in cmake zip sha256sum strip; do
  command -v "$tool" >/dev/null 2>&1 || { printf 'Missing tool: %s\n' "$tool" >&2; exit 1; }
done

rm -rf "$stage" "$dist_dir/$name.zip" "$dist_dir/$name.zip.sha256"
mkdir -p "$stage"

# PCMON_CMAKE_ARGS lets you pass extra options, for example -DCMAKE_C_COMPILER=clang.
# shellcheck disable=SC2086
cmake -S "$source_dir" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release ${PCMON_CMAKE_ARGS:-}
cmake --build "$build_dir" -j "$(nproc)"
ctest --test-dir "$build_dir" --output-on-failure
cmake --install "$build_dir" --prefix "$stage" --strip

if [ ! -x "$stage/bin/pcmon" ]; then
  printf 'bin/pcmon was not built, so there is nothing to release.\n' >&2
  exit 1
fi

install -m 755 "$source_dir/tools/bundle_install.sh" "$stage/install.sh"
install -m 644 "$source_dir/LICENSE" "$source_dir/README.md" "$stage/"

(cd "$dist_dir" && zip -q -r -X "$name.zip" "$name" && sha256sum "$name.zip" >"$name.zip.sha256")
rm -rf "$stage"

printf '\nRelease ready:\n  %s/%s.zip\n  %s/%s.zip.sha256\n' "$dist_dir" "$name" "$dist_dir" "$name"
printf 'It needs the GTK 4 runtime on the target (Ubuntu: sudo apt install libgtk-4-1).\n'

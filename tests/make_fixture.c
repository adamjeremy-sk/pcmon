// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Writes the fake machine to a folder, so other tools (and the Python original) can read it too.

#include <stdio.h>

#include "fixture.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fputs("usage: make-fixture EXISTING_DIR\n", stderr);
        return 2;
    }
    return pc_fixture_build(argv[1]) ? 0 : 1;
}

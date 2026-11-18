// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Builds a fake /sys and /proc tree that looks like the author's PC, for the tests.

#pragma once

#include <glib.h>

// Writes the tree under root (which must exist). Returns FALSE when a file cannot be written.
gboolean pc_fixture_build(const char *root);

// A new temporary directory holding the tree; the caller removes it with pc_fixture_remove.
char *pc_fixture_make(void);
void pc_fixture_remove(char *root);

// Writes one file under root, creating the folders; used by tests that need a variant tree.
gboolean pc_fixture_write(const char *root, const char *relative, const char *content);
gboolean pc_fixture_link(const char *root, const char *relative, const char *target);

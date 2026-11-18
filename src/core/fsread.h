// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Small helpers for reading sysfs and procfs files and listing their directories.

#pragma once

#include <glib.h>

// The text of a file with surrounding whitespace removed, or NULL when it cannot be read.
char *pc_read_text(const char *path);

// The integer in a file; FALSE when the file is missing or does not hold one.
gboolean pc_read_int(const char *path, gint64 *out);

// Whole-string number parsing (surrounding whitespace allowed). Not finite means failure.
gboolean pc_parse_int(const char *text, gint64 *out);
gboolean pc_parse_double(const char *text, double *out);

// The last run of digits in text as a number (0 when there is none), for natural ordering
// of names like hwmon12, card1 and temp3_input.
int pc_tail_number(const char *text);

// Full paths of the entries of dir whose names match the glob pattern (such as "temp*_input"),
// ordered by their trailing number and then by name. Empty when dir does not exist.
GPtrArray *pc_glob_dir(const char *dir, const char *pattern);

// realpath(); a copy of path itself when it cannot be resolved.
char *pc_realpath(const char *path);

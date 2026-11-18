// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Runs a helper program (lspci, nvidia-smi) and returns what it printed, with a time limit.

#pragma once

#include <glib.h>

// Runs argv (argv[0] is looked up in PATH), waits at most timeout_ms and returns its standard
// output. NULL when the program cannot be started, is killed for running too long, or fails.
char *pc_run_capture(char *const *argv, int timeout_ms);

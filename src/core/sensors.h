// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Reads the whole machine into one PcSnapshot; the state needed between reads lives here.

#pragma once

#include "core/types.h"

typedef struct PcSensors PcSensors;

// root is "/" for the real machine, or a fake tree holding sys/ and proc/ (for tests). Only the
// real machine runs lspci and nvidia-smi.
PcSensors *pc_sensors_new(const char *root);
void pc_sensors_free(PcSensors *sensors);

// One reading of CPU, GPUs, fans and RAM. CPU load is measured since the previous call; the
// very first call waits a quarter of a second to have something to compare with.
// Blocks, so call it from a worker thread. The caller frees the snapshot.
PcSnapshot *pc_sensors_collect(PcSensors *sensors);

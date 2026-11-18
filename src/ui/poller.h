// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Reads the sensors on a background thread so a slow driver cannot freeze the window.

#pragma once

#include "core/sensors.h"

typedef struct PcPoller PcPoller;

// Called on the main thread for every snapshot; the callback owns the snapshot and frees it.
typedef void (*PcPollerCallback)(PcSnapshot *snapshot, gpointer user_data);

// Starts a thread that reads sensors every interval_seconds and passes each snapshot to
// callback through the main loop. The poller takes over (and later frees) sensors.
PcPoller *pc_poller_start(PcSensors *sensors, double interval_seconds, PcPollerCallback callback,
                          gpointer user_data);

// Stops the thread (it ends within about a tenth of a second plus any read in progress).
// After this returns the callback is never called again, so user_data may be freed.
// Call from the main thread only.
void pc_poller_stop(PcPoller *poller);

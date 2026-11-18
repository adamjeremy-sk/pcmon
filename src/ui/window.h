// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// The monitor window: sections for CPU, GPUs, fans and memory, refreshed from the poller.

#pragma once

#include <gtk/gtk.h>

#include "core/sensors.h"

// Creates the window. It takes over sensors and reads them every interval_seconds.
// The window cleans up after itself when it is destroyed.
GtkWidget *pc_window_new(GtkApplication *app, PcSensors *sensors, double interval_seconds);

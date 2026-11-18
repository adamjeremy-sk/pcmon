// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// The Options popover: always on top, opacity, the shared-memory line and a name box per fan.

#pragma once

#include <gtk/gtk.h>

#include "core/settings.h"

typedef struct PcOptions PcOptions;

// Called after any control changed; settings already hold the new values.
typedef void (*PcOptionsChanged)(gpointer user_data);

// Builds the controls from settings (which the options keep updating; not owned).
PcOptions *pc_options_new(PcSettings *settings, PcOptionsChanged changed, gpointer user_data);
void pc_options_free(PcOptions *options);

// The "Options" button to place in the window.
GtkWidget *pc_options_button(const PcOptions *options);

// Adds a name box for this fan the first time it is seen. placeholder is the sensor's own label.
void pc_options_ensure_fan(PcOptions *options, const char *fan_id, const char *placeholder);

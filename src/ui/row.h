// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// One reading: a name on the left, a value on the right and an optional coloured bar below.

#pragma once

#include <gtk/gtk.h>

// Fractions at which a bar turns orange and then red.
#define PC_TEMP_WARN 0.75 // as a fraction of the sensor's critical temperature
#define PC_TEMP_CRIT 0.90
#define PC_USAGE_WARN 0.70 // as a fraction of capacity
#define PC_USAGE_CRIT 0.90

typedef struct {
    GtkWidget *box; // add this to a container
    GtkWidget *name;
    GtkWidget *value;
    GtkWidget *bar;
} PcRow;

PcRow *pc_row_new(const char *name);

// Shows text on the right. A frac of 0..1 shows the bar, coloured by the two thresholds;
// a negative frac hides it.
void pc_row_update(PcRow *row, const char *text, double frac, double warn, double crit);

// The widgets belong to their parent; this frees only the PcRow itself.
void pc_row_free(PcRow *row);

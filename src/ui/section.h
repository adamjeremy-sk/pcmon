// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// A titled group of rows that are created on first use and hidden when a sensor goes away.

#pragma once

#include <gtk/gtk.h>

#include "ui/row.h"

typedef struct {
    GtkWidget *container; // the whole section; show or hide it as one
    GtkWidget *title;
    GtkWidget *box;   // holds the rows
    GtkWidget *empty; // shown when no row was reported
    GHashTable *rows; // key -> PcRow*
    GHashTable *seen; // keys reported since pc_section_begin
} PcSection;

// Appends the section to parent. empty_text is what to show when there are no rows.
PcSection *pc_section_new(GtkBox *parent, const char *title, const char *empty_text);

// One update round: begin, then pc_section_row for each reading, then finish.
void pc_section_begin(PcSection *section);

// The row for key (created on first use), with its name set. Marks it as seen.
PcRow *pc_section_row(PcSection *section, const char *key, const char *name);

// Hides the rows that were not reported and shows the empty text if none were.
void pc_section_finish(PcSection *section);

// FALSE when nothing was reported in the last round.
gboolean pc_section_has_rows(const PcSection *section);

void pc_section_free(PcSection *section);

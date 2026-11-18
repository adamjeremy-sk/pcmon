// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Builds a reading row and colours its bar.

#include "ui/row.h"

#include "ui/label.h"

PcRow *pc_row_new(const char *name) {
    PcRow *row = g_new0(PcRow, 1);
    row->box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *line = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    row->name = pc_label_new(name, "name", FALSE, FALSE);
    gtk_widget_set_hexpand(row->name, TRUE);
    row->value = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(row->value), 1.0f);
    gtk_widget_add_css_class(row->value, "value");
    gtk_box_append(GTK_BOX(line), row->name);
    gtk_box_append(GTK_BOX(line), row->value);
    row->bar = gtk_progress_bar_new();
    gtk_widget_set_visible(row->bar, FALSE);
    gtk_box_append(GTK_BOX(row->box), line);
    gtk_box_append(GTK_BOX(row->box), row->bar);
    return row;
}

void pc_row_update(PcRow *row, const char *text, double frac, double warn, double crit) {
    gtk_label_set_text(GTK_LABEL(row->value), text);
    if (frac < 0.0) {
        gtk_widget_set_visible(row->bar, FALSE);
        return;
    }
    gtk_widget_set_visible(row->bar, TRUE);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(row->bar), frac);
    gtk_widget_remove_css_class(row->bar, "ok");
    gtk_widget_remove_css_class(row->bar, "warn");
    gtk_widget_remove_css_class(row->bar, "crit");
    gtk_widget_add_css_class(row->bar, frac >= crit ? "crit" : (frac >= warn ? "warn" : "ok"));
}

void pc_row_free(PcRow *row) {
    g_free(row);
}

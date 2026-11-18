// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Creates sections and keeps their rows in step with the readings.

#include "ui/section.h"

#include "ui/label.h"

PcSection *pc_section_new(GtkBox *parent, const char *title, const char *empty_text) {
    PcSection *s = g_new0(PcSection, 1);
    s->container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    s->title = pc_label_new(title, "title", FALSE, TRUE);
    s->box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    s->empty = pc_label_new(empty_text, "dim", TRUE, FALSE);
    gtk_widget_set_visible(s->empty, FALSE);
    gtk_box_append(GTK_BOX(s->container), s->title);
    gtk_box_append(GTK_BOX(s->container), s->box);
    gtk_box_append(GTK_BOX(s->container), s->empty);
    gtk_box_append(parent, s->container);
    s->rows = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)pc_row_free);
    s->seen = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    return s;
}

void pc_section_begin(PcSection *section) {
    g_hash_table_remove_all(section->seen);
}

PcRow *pc_section_row(PcSection *section, const char *key, const char *name) {
    PcRow *row = g_hash_table_lookup(section->rows, key);
    if (row == NULL) {
        row = pc_row_new(name);
        g_hash_table_insert(section->rows, g_strdup(key), row);
        gtk_box_append(GTK_BOX(section->box), row->box);
    }
    gtk_label_set_text(GTK_LABEL(row->name), name);
    g_hash_table_add(section->seen, g_strdup(key));
    return row;
}

void pc_section_finish(PcSection *section) {
    GHashTableIter it;
    gpointer key, value;
    g_hash_table_iter_init(&it, section->rows);
    while (g_hash_table_iter_next(&it, &key, &value)) {
        PcRow *row = value;
        gtk_widget_set_visible(row->box, g_hash_table_contains(section->seen, key));
    }
    gtk_widget_set_visible(section->empty, g_hash_table_size(section->seen) == 0);
}

gboolean pc_section_has_rows(const PcSection *section) {
    return g_hash_table_size(section->seen) > 0;
}

void pc_section_free(PcSection *section) {
    if (section == NULL) {
        return;
    }
    g_hash_table_unref(section->rows);
    g_hash_table_unref(section->seen);
    g_free(section);
}

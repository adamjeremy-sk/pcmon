// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Creates the labels used by the rows, sections and options.

#include "ui/label.h"

GtkWidget *pc_label_new(const char *text, const char *css_class, gboolean wrap,
                        gboolean ellipsize) {
    GtkWidget *label = gtk_label_new(text);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
    if (css_class != NULL) {
        gtk_widget_add_css_class(label, css_class);
    }
    if (wrap) {
        gtk_label_set_wrap(GTK_LABEL(label), TRUE);
    }
    if (ellipsize) {
        gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
        gtk_label_set_max_width_chars(GTK_LABEL(label), 36);
    }
    return label;
}

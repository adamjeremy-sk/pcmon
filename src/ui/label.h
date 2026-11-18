// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// One helper that makes the left-aligned labels the window is built from.

#pragma once

#include <gtk/gtk.h>

// A left-aligned label with an optional CSS class (NULL for none), optional word wrapping
// and optional end ellipsis (about 36 characters wide).
GtkWidget *pc_label_new(const char *text, const char *css_class, gboolean wrap, gboolean ellipsize);

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Loads the stylesheet into a CSS provider for the whole display.

#include "ui/style.h"

#include <gtk/gtk.h>

static const char kCss[] =
    ".title { font-size: 13px; font-weight: bold; opacity: 0.75; margin-top: 4px; }\n"
    ".name { font-size: 14px; }\n"
    ".value { font-size: 15px; font-weight: bold; }\n"
    ".dim { opacity: 0.65; font-size: 12px; }\n"
    "progressbar trough, progressbar progress { min-height: 8px; border-radius: 4px; }\n"
    "progressbar.ok progress { background-image: none; background-color: #2e9e5b; }\n"
    "progressbar.warn progress { background-image: none; background-color: #e0962b; }\n"
    "progressbar.crit progress { background-image: none; background-color: #d64541; }\n";

void pc_style_install(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider, kCss);
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                               GTK_STYLE_PROVIDER(provider),
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider); // the display keeps its own reference
}

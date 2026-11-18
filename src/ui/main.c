// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Program entry point: command line options and the GtkApplication.

#include <gtk/gtk.h>

#include "core/sensors.h"
#include "ui/style.h"
#include "ui/window.h"

#define APP_ID "io.github.adamjeremy_sk.Pcmon"
#define MIN_INTERVAL 0.1

static double option_interval = 1.0;
static char *option_root = NULL;

static const GOptionEntry entries[] = {
    {"interval", 0, 0, G_OPTION_ARG_DOUBLE, &option_interval, "Seconds between updates (default 1)",
     "SECONDS"},
    // Hidden: read a fake /sys and /proc tree instead of the real ones, for testing.
    {"root", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_FILENAME, &option_root, NULL, "DIR"},
    {NULL, 0, 0, 0, NULL, NULL, NULL},
};

static void on_activate(GApplication *app, gpointer data) {
    (void)data;
    GList *windows = gtk_application_get_windows(GTK_APPLICATION(app));
    if (windows != NULL) {
        gtk_window_present(GTK_WINDOW(windows->data)); // a second launch raises the first window
        return;
    }
    gtk_window_set_default_icon_name("pcmon"); // lets the taskbar pair the window with its icon
    pc_style_install();
    PcSensors *sensors = pc_sensors_new(option_root != NULL ? option_root : "/");
    double interval = option_interval < MIN_INTERVAL ? MIN_INTERVAL : option_interval;
    gtk_window_present(GTK_WINDOW(pc_window_new(GTK_APPLICATION(app), sensors, interval)));
}

int main(int argc, char **argv) {
    g_set_prgname("pcmon");
    g_set_application_name("pcmon");
    GtkApplication *app = gtk_application_new(APP_ID, G_APPLICATION_DEFAULT_FLAGS);
    g_application_add_main_option_entries(G_APPLICATION(app), entries);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    g_free(option_root);
    return status;
}

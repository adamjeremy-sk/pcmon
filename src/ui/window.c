// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Draws each snapshot into the sections and keeps the settings file up to date.

#include "ui/window.h"

#include <stdio.h>
#include <time.h>

#include "core/format.h"
#include "core/settings.h"
#include "ui/keep_above.h"
#include "ui/label.h"
#include "ui/options.h"
#include "ui/poller.h"
#include "ui/section.h"

#define GIB (1024.0 * 1024.0 * 1024.0)

typedef struct {
    GtkWidget *window;
    PcSettings *settings;
    char *settings_path;
    PcOptions *options;
    PcPoller *poller;
    PcSnapshot *last; // newest snapshot, kept so a changed option can redraw at once
    PcSection *cpu;
    GtkWidget *gpu_box;
    GtkWidget *no_gpu;
    GHashTable *gpu_sections; // key -> PcSection*
    PcSection *fans;
    PcSection *mem;
    GtkWidget *status;
    gboolean system_dark; // the desktop's own choice, restored when the theme is "System"
} PcWindow;

static void render_cpu(PcWindow *w, const PcCpu *cpu) {
    char *title = cpu->name != NULL && cpu->name[0] != '\0'
                      ? g_strdup_printf("CPU  ·  %s", cpu->name)
                      : g_strdup("CPU");
    gtk_label_set_text(GTK_LABEL(w->cpu->title), title);
    g_free(title);
    pc_section_begin(w->cpu);
    if (cpu->load.set) {
        char *text = g_strdup_printf("%.0f%%", cpu->load.value);
        pc_row_update(pc_section_row(w->cpu, "load", "Load"), text, cpu->load.value / 100,
                      PC_USAGE_WARN, PC_USAGE_CRIT);
        g_free(text);
    }
    if (cpu->has_temp) {
        char *text = g_strdup_printf("%.0f °C", cpu->temp.value);
        pc_row_update(pc_section_row(w->cpu, "temp", "Temperature"), text, pc_temp_frac(&cpu->temp),
                      PC_TEMP_WARN, PC_TEMP_CRIT);
        g_free(text);
    }
    pc_section_finish(w->cpu);
}

static PcSection *gpu_section(PcWindow *w, const PcGpu *gpu, guint index) {
    char *key = gpu->slot != NULL ? g_strdup(gpu->slot) : g_strdup_printf("%u", index);
    PcSection *sec = g_hash_table_lookup(w->gpu_sections, key);
    if (sec == NULL) {
        sec =
            pc_section_new(GTK_BOX(w->gpu_box), gpu->name, "No sensor data exposed by the driver");
        g_hash_table_insert(w->gpu_sections, key, sec);
    } else {
        g_free(key);
    }
    return sec;
}

static void render_gpu(PcWindow *w, const PcGpu *gpu, guint index) {
    PcSection *sec = gpu_section(w, gpu, index);
    gtk_label_set_text(GTK_LABEL(sec->title), gpu->name);
    gtk_widget_set_tooltip_text(sec->title, gpu->name);
    pc_section_begin(sec);
    if (gpu->load.set) {
        char *text = g_strdup_printf("%.0f%%", gpu->load.value);
        pc_row_update(pc_section_row(sec, "load", "Load"), text, gpu->load.value / 100,
                      PC_USAGE_WARN, PC_USAGE_CRIT);
        g_free(text);
    }
    for (guint i = 0; i < gpu->temps->len; i++) {
        const PcTemp *t = g_ptr_array_index(gpu->temps, i);
        char *key = g_strconcat("t:", t->label, NULL);
        char *name = pc_temp_name(t->label);
        char *text = g_strdup_printf("%.0f °C", t->value);
        pc_row_update(pc_section_row(sec, key, name), text, pc_temp_frac(t), PC_TEMP_WARN,
                      PC_TEMP_CRIT);
        g_free(key);
        g_free(name);
        g_free(text);
    }
    if (gpu->vram_total.set && gpu->vram_total.value != 0) {
        double used = gpu->vram_used.set ? gpu->vram_used.value : 0;
        char *text = g_strdup_printf("%.1f / %.1f GiB", used / GIB, gpu->vram_total.value / GIB);
        pc_row_update(pc_section_row(sec, "vram", "VRAM"), text, used / gpu->vram_total.value,
                      PC_USAGE_WARN, PC_USAGE_CRIT);
        g_free(text);
    } else if (gpu->shared_memory && w->settings->show_shared_memory) {
        pc_row_update(pc_section_row(sec, "vram", "VRAM"), "shared with system RAM", -1, 0, 0);
    }
    for (guint i = 0; i < gpu->fans->len; i++) {
        const PcFan *f = g_ptr_array_index(gpu->fans, i);
        char *key = g_strconcat("f:", f->label, NULL);
        char *name = pc_fan_name(f, gpu->fans->len);
        char *text = pc_format_fan(f);
        pc_row_update(pc_section_row(sec, key, name), text, -1, 0, 0);
        g_free(key);
        g_free(name);
        g_free(text);
    }
    pc_section_finish(sec);
    // An integrated GPU with nothing to show (line turned off) is hidden entirely.
    gtk_widget_set_visible(sec->container, pc_section_has_rows(sec) || !gpu->shared_memory);
}

static void render_fans(PcWindow *w, const PcSnapshot *snap) {
    pc_section_begin(w->fans);
    for (guint i = 0; i < snap->fans->len; i++) {
        const PcFan *f = g_ptr_array_index(snap->fans, i);
        char *id = pc_fan_id(f);
        pc_options_ensure_fan(w->options, id, f->label);
        const char *name = g_hash_table_lookup(w->settings->fan_names, id);
        if (name != NULL || f->value > 0) {
            char *text = pc_format_fan(f);
            pc_row_update(pc_section_row(w->fans, f->label, name != NULL ? name : f->label), text,
                          -1, 0, 0);
            g_free(text);
        }
        g_free(id);
    }
    pc_section_finish(w->fans);
}

static void render_memory(PcWindow *w, const PcRam *ram) {
    pc_section_begin(w->mem);
    if (ram->present && ram->total > 0) {
        double frac = ram->used / ram->total;
        char *text = g_strdup_printf("%.1f / %.1f GiB  (%.0f%%)", ram->used / GIB, ram->total / GIB,
                                     frac * 100);
        pc_row_update(pc_section_row(w->mem, "ram", "RAM"), text, frac, PC_USAGE_WARN,
                      PC_USAGE_CRIT);
        g_free(text);
    }
    pc_section_finish(w->mem);
}

static void render(PcWindow *w, const PcSnapshot *snap) {
    render_cpu(w, &snap->cpu);
    gtk_widget_set_visible(w->no_gpu, snap->gpus->len == 0);
    for (guint i = 0; i < snap->gpus->len; i++) {
        render_gpu(w, g_ptr_array_index(snap->gpus, i), i);
    }
    render_fans(w, snap);
    render_memory(w, &snap->ram);
    char stamp[16];
    time_t now = time(NULL);
    struct tm local;
    localtime_r(&now, &local);
    strftime(stamp, sizeof stamp, "%H:%M:%S", &local);
    char *text = g_strdup_printf("updated %s", stamp);
    gtk_label_set_text(GTK_LABEL(w->status), text);
    g_free(text);
}

// Runs on the main thread for every reading; takes ownership of snap.
static void on_snapshot(PcSnapshot *snap, gpointer data) {
    PcWindow *w = data;
    pc_snapshot_free(w->last);
    w->last = snap;
    render(w, snap);
}

static void apply_theme(PcWindow *w) {
    gboolean dark = w->settings->theme == PC_THEME_DARK ||
                    (w->settings->theme == PC_THEME_SYSTEM && w->system_dark);
    g_object_set(gtk_settings_get_default(), "gtk-application-prefer-dark-theme", dark, NULL);
}

static void apply_window_state(PcWindow *w) {
    apply_theme(w);
    gboolean on = w->settings->always_on_top && pc_keep_above_supported();
    pc_keep_above_set(GTK_WINDOW(w->window), on);
    // Normal windows stay opaque; the slider only matters while the window is on top.
    gtk_widget_set_opacity(w->window, on ? w->settings->opacity / 100.0 : 1.0);
}

// Called by the options popover after any control changed.
static void on_options_changed(gpointer data) {
    PcWindow *w = data;
    apply_window_state(w);
    pc_settings_save(w->settings, w->settings_path);
    if (w->last != NULL) {
        render(w, w->last); // show the change now, not at the next update
    }
}

static void on_map(GtkWidget *widget, gpointer data) {
    (void)widget;
    apply_window_state(data);
}

static void on_destroy(GtkWidget *widget, gpointer data) {
    (void)widget;
    PcWindow *w = data;
    pc_poller_stop(w->poller); // after this the snapshot callback never runs again
    pc_snapshot_free(w->last);
    pc_options_free(w->options);
    g_hash_table_unref(w->gpu_sections);
    pc_section_free(w->cpu);
    pc_section_free(w->fans);
    pc_section_free(w->mem);
    pc_settings_free(w->settings);
    g_free(w->settings_path);
    g_free(w);
}

static void on_close_clicked(GtkButton *button, gpointer data) {
    (void)button;
    gtk_window_destroy(GTK_WINDOW(data));
}

GtkWidget *pc_window_new(GtkApplication *app, PcSensors *sensors, double interval_seconds) {
    PcWindow *w = g_new0(PcWindow, 1);
    w->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(w->window), "pcmon");
    gtk_window_set_default_size(GTK_WINDOW(w->window), 380, -1);

    g_object_get(gtk_settings_get_default(), "gtk-application-prefer-dark-theme", &w->system_dark,
                 NULL);
    char *legacy = pc_settings_legacy_path();
    w->settings_path = pc_settings_default_path();
    w->settings = pc_settings_load(w->settings_path, legacy);
    g_free(legacy);
    w->gpu_sections =
        g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)pc_section_free);

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(root, 18);
    gtk_widget_set_margin_bottom(root, 18);
    gtk_widget_set_margin_start(root, 18);
    gtk_widget_set_margin_end(root, 18);
    gtk_window_set_child(GTK_WINDOW(w->window), root);

    w->cpu = pc_section_new(GTK_BOX(root), "CPU", "No CPU temperature sensor found");
    w->gpu_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_append(GTK_BOX(root), w->gpu_box);
    w->no_gpu = pc_label_new("No GPU found", "dim", FALSE, FALSE);
    gtk_widget_set_visible(w->no_gpu, FALSE);
    gtk_box_append(GTK_BOX(root), w->no_gpu);
    w->fans =
        pc_section_new(GTK_BOX(root), "Fans",
                       "No fan sensors found. Motherboard fans need a kernel driver (see README).");
    w->mem = pc_section_new(GTK_BOX(root), "Memory", "Memory info unavailable");
    w->status = pc_label_new("", "dim", TRUE, FALSE);
    gtk_widget_set_margin_top(w->status, 4);
    gtk_box_append(GTK_BOX(root), w->status);

    w->options = pc_options_new(w->settings, on_options_changed, w);
    GtkWidget *close = gtk_button_new_with_label("Close");
    g_signal_connect(close, "clicked", G_CALLBACK(on_close_clicked), w->window);
    GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer, TRUE);
    GtkWidget *bottom = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(bottom), pc_options_button(w->options));
    gtk_box_append(GTK_BOX(bottom), spacer);
    gtk_box_append(GTK_BOX(bottom), close);
    gtk_box_append(GTK_BOX(root), bottom);

    g_signal_connect(w->window, "map", G_CALLBACK(on_map), w);
    g_signal_connect(w->window, "destroy", G_CALLBACK(on_destroy), w);
    apply_window_state(w);
    w->poller = pc_poller_start(sensors, interval_seconds, on_snapshot, w);
    return w->window;
}

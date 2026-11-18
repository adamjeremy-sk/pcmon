// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Builds the Options popover and writes its controls back into the settings.

#include "ui/options.h"

#include "ui/keep_above.h"
#include "ui/label.h"

struct PcOptions {
    PcSettings *settings;
    PcOptionsChanged changed;
    gpointer user_data;
    GtkWidget *button;
    GtkWidget *top;
    GtkWidget *opacity;
    GtkWidget *shared;
    GtkWidget *theme;
    GtkWidget *fan_group; // hidden until a fan is detected
    GtkWidget *fan_box;
    GHashTable *fan_entries; // fan id -> GtkEntry (owned by the popover)
};

static void notify(PcOptions *o) {
    gtk_widget_set_sensitive(o->opacity, o->settings->always_on_top);
    o->changed(o->user_data);
}

static void on_top_toggled(GtkCheckButton *button, gpointer data) {
    PcOptions *o = data;
    o->settings->always_on_top = gtk_check_button_get_active(button);
    notify(o);
}

static void on_shared_toggled(GtkCheckButton *button, gpointer data) {
    PcOptions *o = data;
    o->settings->show_shared_memory = gtk_check_button_get_active(button);
    notify(o);
}

static void on_theme_changed(GObject *dropdown, GParamSpec *pspec, gpointer data) {
    (void)pspec;
    PcOptions *o = data;
    o->settings->theme = (PcTheme)gtk_drop_down_get_selected(GTK_DROP_DOWN(dropdown));
    notify(o);
}

static void on_opacity_changed(GtkRange *range, gpointer data) {
    PcOptions *o = data;
    o->settings->opacity = (int)gtk_range_get_value(range);
    notify(o);
}

static char *format_percent(GtkScale *scale, double value, gpointer data) {
    (void)scale;
    (void)data;
    return g_strdup_printf("%.0f%%", value);
}

typedef struct {
    PcOptions *options;
    char *fan_id;
} FanEntryData;

static void fan_entry_data_free(gpointer data, GClosure *closure) {
    (void)closure;
    FanEntryData *d = data;
    g_free(d->fan_id);
    g_free(d);
}

static void on_fan_renamed(GtkEditable *editable, gpointer data) {
    FanEntryData *d = data;
    pc_settings_set_fan_name(d->options->settings, d->fan_id, gtk_editable_get_text(editable));
    d->options->changed(d->options->user_data);
}

void pc_options_ensure_fan(PcOptions *o, const char *fan_id, const char *placeholder) {
    if (g_hash_table_contains(o->fan_entries, fan_id)) {
        return;
    }
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *id_label = pc_label_new(fan_id, "dim", FALSE, FALSE);
    gtk_widget_set_hexpand(id_label, TRUE);
    GtkWidget *entry = gtk_entry_new();
    g_object_set(entry, "placeholder-text", placeholder, NULL);
    gtk_editable_set_width_chars(GTK_EDITABLE(entry), 16);
    const char *name = g_hash_table_lookup(o->settings->fan_names, fan_id);
    gtk_editable_set_text(GTK_EDITABLE(entry), name != NULL ? name : "");
    FanEntryData *d = g_new0(FanEntryData, 1);
    d->options = o;
    d->fan_id = g_strdup(fan_id);
    // Connected after set_text on purpose: filling in the saved name is not a change.
    g_signal_connect_data(entry, "changed", G_CALLBACK(on_fan_renamed), d, fan_entry_data_free, 0);
    gtk_box_append(GTK_BOX(row), id_label);
    gtk_box_append(GTK_BOX(row), entry);
    gtk_box_append(GTK_BOX(o->fan_box), row);
    g_hash_table_insert(o->fan_entries, g_strdup(fan_id), entry);
    gtk_widget_set_visible(o->fan_group, TRUE);
}

PcOptions *pc_options_new(PcSettings *settings, PcOptionsChanged changed, gpointer user_data) {
    PcOptions *o = g_new0(PcOptions, 1);
    o->settings = settings;
    o->changed = changed;
    o->user_data = user_data;
    o->fan_entries = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

    static const char *const theme_names[] = {"System", "Light", "Dark", NULL};
    o->theme = gtk_drop_down_new_from_strings(theme_names);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(o->theme), settings->theme);
    GtkWidget *theme_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *theme_label = pc_label_new("Theme", NULL, FALSE, FALSE);
    gtk_widget_set_hexpand(theme_label, TRUE);
    gtk_box_append(GTK_BOX(theme_row), theme_label);
    gtk_box_append(GTK_BOX(theme_row), o->theme);

    o->top = gtk_check_button_new_with_label("Always on top");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(o->top), settings->always_on_top);
    if (!pc_keep_above_supported()) {
        gtk_widget_set_sensitive(o->top, FALSE);
        gtk_widget_set_tooltip_text(
            o->top, "Needs an X11 session; GTK 4 has no way to do this on Wayland.");
    }
    o->opacity = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, PC_MIN_OPACITY, 100, 5);
    gtk_range_set_value(GTK_RANGE(o->opacity), settings->opacity);
    gtk_scale_set_draw_value(GTK_SCALE(o->opacity), TRUE);
    gtk_scale_set_value_pos(GTK_SCALE(o->opacity), GTK_POS_RIGHT);
    gtk_scale_set_format_value_func(GTK_SCALE(o->opacity), format_percent, NULL, NULL);
    gtk_widget_set_size_request(o->opacity, 220, -1);
    gtk_widget_set_sensitive(o->opacity, settings->always_on_top);
    o->shared = gtk_check_button_new_with_label("Show “shared with system RAM”");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(o->shared), settings->show_shared_memory);

    GtkWidget *hint = pc_label_new("Name a fan to always show it, even at 0 RPM. "
                                   "Clear the name to hide it while stopped.",
                                   "dim", TRUE, FALSE);
    gtk_label_set_max_width_chars(GTK_LABEL(hint), 38);
    o->fan_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    o->fan_group = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_box_append(GTK_BOX(o->fan_group), pc_label_new("Fan names", "title", FALSE, FALSE));
    gtk_box_append(GTK_BOX(o->fan_group), hint);
    gtk_box_append(GTK_BOX(o->fan_group), o->fan_box);
    gtk_widget_set_visible(o->fan_group, FALSE); // until a fan is detected

    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_top(content, 12);
    gtk_widget_set_margin_bottom(content, 12);
    gtk_widget_set_margin_start(content, 12);
    gtk_widget_set_margin_end(content, 12);
    gtk_box_append(GTK_BOX(content), theme_row);
    gtk_box_append(GTK_BOX(content), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    gtk_box_append(GTK_BOX(content), o->top);
    gtk_box_append(GTK_BOX(content), pc_label_new("Opacity (when on top)", NULL, FALSE, FALSE));
    gtk_box_append(GTK_BOX(content), o->opacity);
    gtk_box_append(GTK_BOX(content), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    gtk_box_append(GTK_BOX(content), o->shared);
    gtk_box_append(GTK_BOX(content), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    gtk_box_append(GTK_BOX(content), o->fan_group);

    GtkWidget *popover = gtk_popover_new();
    gtk_popover_set_child(GTK_POPOVER(popover), content);
    o->button = gtk_menu_button_new();
    gtk_menu_button_set_label(GTK_MENU_BUTTON(o->button), "Options");
    gtk_menu_button_set_popover(GTK_MENU_BUTTON(o->button), popover);
    gtk_menu_button_set_direction(GTK_MENU_BUTTON(o->button), GTK_ARROW_UP);

    g_signal_connect(o->top, "toggled", G_CALLBACK(on_top_toggled), o);
    g_signal_connect(o->opacity, "value-changed", G_CALLBACK(on_opacity_changed), o);
    g_signal_connect(o->theme, "notify::selected", G_CALLBACK(on_theme_changed), o);
    g_signal_connect(o->shared, "toggled", G_CALLBACK(on_shared_toggled), o);
    return o;
}

GtkWidget *pc_options_button(const PcOptions *options) {
    return options->button;
}

void pc_options_free(PcOptions *options) {
    if (options == NULL) {
        return;
    }
    g_hash_table_unref(options->fan_entries);
    g_free(options);
}

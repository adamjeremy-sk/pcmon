// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Reads and writes settings.ini with GKeyFile.

#include "core/settings.h"

#include <glib/gstdio.h>

#include "core/fsread.h"

static void set_defaults_fan_names(GHashTable *names) {
    g_hash_table_replace(names, g_strdup("fan1"), g_strdup("CPU Tower"));
    g_hash_table_replace(names, g_strdup("fan5"), g_strdup("Front intake"));
    g_hash_table_replace(names, g_strdup("fan6"), g_strdup("Rear exhaust"));
    g_hash_table_replace(names, g_strdup("fan7"), g_strdup("Top case"));
}

PcSettings *pc_settings_default(void) {
    PcSettings *s = g_new0(PcSettings, 1);
    s->always_on_top = FALSE;
    s->opacity = 100;
    s->show_shared_memory = TRUE;
    s->theme = PC_THEME_SYSTEM;
    s->fan_names = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    set_defaults_fan_names(s->fan_names);
    return s;
}

void pc_settings_free(PcSettings *settings) {
    if (settings == NULL) {
        return;
    }
    g_hash_table_unref(settings->fan_names);
    g_free(settings);
}

void pc_settings_set_fan_name(PcSettings *settings, const char *fan_id, const char *name) {
    char *trimmed = g_strstrip(g_strdup(name != NULL ? name : ""));
    if (trimmed[0] == '\0') {
        g_hash_table_remove(settings->fan_names, fan_id);
        g_free(trimmed);
    } else {
        g_hash_table_replace(settings->fan_names, g_strdup(fan_id), trimmed);
    }
}

static gboolean truthy(const char *text) {
    char *copy = g_strstrip(g_strdup(text));
    char *low = g_ascii_strdown(copy, -1);
    gboolean yes = g_str_equal(low, "1") || g_str_equal(low, "true") || g_str_equal(low, "yes") ||
                   g_str_equal(low, "on");
    g_free(low);
    g_free(copy);
    return yes;
}

static gboolean get_bool(GKeyFile *kf, const char *group, const char *key, gboolean fallback) {
    char *text = g_key_file_get_string(kf, group, key, NULL);
    gboolean value = text != NULL ? truthy(text) : fallback;
    g_free(text);
    return value;
}

static int get_int(GKeyFile *kf, const char *group, const char *key, int fallback) {
    char *text = g_key_file_get_string(kf, group, key, NULL);
    gint64 value;
    int result = (text != NULL && pc_parse_int(text, &value)) ? (int)CLAMP(value, -1000000, 1000000)
                                                              : fallback;
    g_free(text);
    return result;
}

static const char *const kThemeNames[] = {"system", "light", "dark"};

static PcTheme get_theme(GKeyFile *kf) {
    char *text = g_key_file_get_string(kf, "window", "theme", NULL);
    PcTheme theme = PC_THEME_SYSTEM;
    if (text != NULL) {
        char *low = g_ascii_strdown(g_strstrip(text), -1);
        for (guint i = 0; i < G_N_ELEMENTS(kThemeNames); i++) {
            if (g_str_equal(low, kThemeNames[i])) {
                theme = (PcTheme)i;
            }
        }
        g_free(low);
    }
    g_free(text);
    return theme;
}

static gboolean load_file(PcSettings *s, const char *path) {
    GKeyFile *kf = g_key_file_new();
    gboolean ok = path != NULL && g_key_file_load_from_file(kf, path, G_KEY_FILE_NONE, NULL);
    if (ok) {
        s->always_on_top = get_bool(kf, "window", "always_on_top", FALSE);
        s->opacity = CLAMP(get_int(kf, "window", "opacity", 100), PC_MIN_OPACITY, 100);
        s->theme = get_theme(kf);
        s->show_shared_memory = get_bool(kf, "display", "show_shared_memory", TRUE);
        if (g_key_file_has_group(kf, "fans")) { // once saved, this list replaces the defaults
            g_hash_table_remove_all(s->fan_names);
            gsize count = 0;
            char **keys = g_key_file_get_keys(kf, "fans", &count, NULL);
            for (gsize i = 0; keys != NULL && i < count; i++) {
                char *value = g_key_file_get_string(kf, "fans", keys[i], NULL);
                pc_settings_set_fan_name(s, keys[i], value);
                g_free(value);
            }
            g_strfreev(keys);
        }
    }
    g_key_file_free(kf);
    return ok;
}

PcSettings *pc_settings_load(const char *path, const char *fallback) {
    PcSettings *s = pc_settings_default();
    if (!load_file(s, path)) {
        PcSettings *fresh = pc_settings_default();
        pc_settings_free(s);
        s = fresh;
        load_file(s, fallback);
    }
    return s;
}

static int compare_keys(gconstpointer a, gconstpointer b) {
    return g_strcmp0(*(const char *const *)a, *(const char *const *)b);
}

void pc_settings_save(const PcSettings *settings, const char *path) {
    GKeyFile *kf = g_key_file_new();
    g_key_file_set_string(kf, "window", "always_on_top",
                          settings->always_on_top ? "true" : "false");
    g_key_file_set_string(kf, "window", "theme", kThemeNames[settings->theme]);
    char *opacity = g_strdup_printf("%d", settings->opacity);
    g_key_file_set_string(kf, "window", "opacity", opacity);
    g_free(opacity);
    g_key_file_set_string(kf, "display", "show_shared_memory",
                          settings->show_shared_memory ? "true" : "false");
    GPtrArray *keys = g_ptr_array_new();
    GHashTableIter it;
    gpointer key, value;
    g_hash_table_iter_init(&it, settings->fan_names);
    while (g_hash_table_iter_next(&it, &key, &value)) {
        g_ptr_array_add(keys, key);
    }
    g_ptr_array_sort(keys, compare_keys);
    for (guint i = 0; i < keys->len; i++) {
        const char *k = g_ptr_array_index(keys, i);
        g_key_file_set_string(kf, "fans", k, g_hash_table_lookup(settings->fan_names, k));
    }
    gboolean no_fans = keys->len == 0;
    g_ptr_array_unref(keys);

    char *dir = g_path_get_dirname(path);
    g_mkdir_with_parents(dir, 0755);
    g_free(dir);
    char *data = g_key_file_to_data(kf, NULL, NULL);
    if (no_fans) {
        // g_key_file cannot hold an empty group, so append the bare header by hand.
        char *with_group = g_strconcat(data, "\n[fans]\n", NULL);
        g_free(data);
        data = with_group;
    }
    g_file_set_contents(path, data, -1, NULL);
    g_free(data);
    g_key_file_free(kf);
}

static char *config_path(const char *app) {
    return g_build_filename(g_get_user_config_dir(), app, "settings.ini", NULL);
}

char *pc_settings_default_path(void) {
    return config_path("pcmon");
}

char *pc_settings_legacy_path(void) {
    return config_path("pymon");
}

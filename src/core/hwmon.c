// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Reads the temp*_input and fan*_input files of a hwmon directory.

#include "core/hwmon.h"

#include "core/fsread.h"

// The text of dir/<prefix><index><suffix>, or NULL.
static char *read_indexed(const char *dir, const char *prefix, int index, const char *suffix) {
    char *name = g_strdup_printf("%s%d%s", prefix, index, suffix);
    char *path = g_build_filename(dir, name, NULL);
    char *text = pc_read_text(path);
    g_free(path);
    g_free(name);
    return text;
}

static gboolean read_indexed_int(const char *dir, const char *prefix, int index, const char *suffix,
                                 gint64 *out) {
    char *text = read_indexed(dir, prefix, index, suffix);
    gboolean ok = pc_parse_int(text, out);
    g_free(text);
    return ok;
}

// Label from <prefix><index>_label, or "<prefix><index>" when there is none or it is empty.
static char *label_or_default(const char *dir, const char *prefix, int index) {
    char *label = read_indexed(dir, prefix, index, "_label");
    if (label != NULL && label[0] != '\0') {
        return label;
    }
    g_free(label);
    return g_strdup_printf("%s%d", prefix, index);
}

GPtrArray *pc_hwmon_temps(const char *dir) {
    GPtrArray *out = pc_temp_array_new();
    GPtrArray *paths = pc_glob_dir(dir, "temp*_input");
    for (guint i = 0; i < paths->len; i++) {
        const char *path = g_ptr_array_index(paths, i);
        int index = pc_tail_number(path);
        gint64 milli;
        if (!pc_read_int(path, &milli)) {
            continue;
        }
        gint64 crit = 0;
        gboolean have = read_indexed_int(dir, "temp", index, "_crit", &crit) && crit != 0;
        if (!have) {
            have = read_indexed_int(dir, "temp", index, "_max", &crit) && crit != 0;
        }
        char *label = label_or_default(dir, "temp", index);
        PcTemp *t = pc_temp_new(label, milli / 1000.0, have ? pc_some(crit / 1000.0) : pc_none());
        g_free(label);
        g_ptr_array_add(out, t);
    }
    g_ptr_array_unref(paths);
    return out;
}

GPtrArray *pc_hwmon_fans(const char *dir, const char *prefix) {
    GPtrArray *out = pc_fan_array_new();
    GPtrArray *paths = pc_glob_dir(dir, "fan*_input");
    for (guint i = 0; i < paths->len; i++) {
        const char *path = g_ptr_array_index(paths, i);
        int index = pc_tail_number(path);
        gint64 rpm;
        if (!pc_read_int(path, &rpm)) {
            continue;
        }
        char *label = label_or_default(dir, "fan", index);
        char *full = (prefix != NULL && prefix[0] != '\0') ? g_strdup_printf("%s %s", prefix, label)
                                                           : g_strdup(label);
        g_strstrip(full);
        g_ptr_array_add(out, pc_fan_new(full, (double)rpm, FALSE));
        g_free(full);
        g_free(label);
    }
    g_ptr_array_unref(paths);
    return out;
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Parses CPU load and finds the CPU temperature sensor.

#include "core/cpu.h"

#include <string.h>

#include "core/fsread.h"
#include "core/hwmon.h"

gboolean pc_parse_cpu_times(const char *text, PcCpuTimes *out) {
    if (text == NULL) {
        return FALSE;
    }
    gboolean result = FALSE;
    char **lines = g_strsplit(text, "\n", -1);
    for (char **line = lines; *line != NULL; line++) {
        if (!g_str_has_prefix(*line, "cpu ")) {
            continue;
        }
        char **fields = g_strsplit_set(*line, " \t", -1);
        gint64 values[8];
        int count = 0;
        gboolean valid = TRUE;
        int seen = 0;
        for (char **f = fields; *f != NULL && count < 8; f++) {
            if ((*f)[0] == '\0') {
                continue;
            }
            if (seen++ == 0) {
                continue; // the "cpu" word itself
            }
            if (!pc_parse_int(*f, &values[count])) {
                valid = FALSE;
                break;
            }
            count++;
        }
        g_strfreev(fields);
        if (valid && count >= 4) {
            gint64 total = 0;
            for (int i = 0; i < count; i++) {
                total += values[i];
            }
            out->total = total;
            out->idle = values[3] + (count > 4 ? values[4] : 0);
            result = TRUE;
        }
        break; // only the first "cpu " line counts, valid or not
    }
    g_strfreev(lines);
    return result;
}

gboolean pc_cpu_load(const PcCpuTimes *prev, const PcCpuTimes *cur, double *out) {
    gint64 d_total = cur->total - prev->total;
    gint64 d_idle = cur->idle - prev->idle;
    if (d_total <= 0) {
        return FALSE;
    }
    double load = 100.0 * (1.0 - (double)d_idle / (double)d_total);
    *out = load < 0.0 ? 0.0 : (load > 100.0 ? 100.0 : load);
    return TRUE;
}

char *pc_cpu_name(const char *proc_root) {
    char *path = g_build_filename(proc_root, "cpuinfo", NULL);
    char *text = pc_read_text(path);
    g_free(path);
    char *name = g_strdup("");
    if (text != NULL) {
        char **lines = g_strsplit(text, "\n", -1);
        for (char **line = lines; *line != NULL; line++) {
            if (!g_str_has_prefix(*line, "model name")) {
                continue;
            }
            const char *colon = strchr(*line, ':');
            if (colon != NULL) {
                // g_strsplit_set leaves empty pieces for repeated separators; squeeze them out.
                GString *out = g_string_new(NULL);
                char **pieces = g_strsplit_set(colon + 1, " \t", -1);
                for (char **p = pieces; *p != NULL; p++) {
                    if ((*p)[0] == '\0') {
                        continue;
                    }
                    if (out->len > 0) {
                        g_string_append_c(out, ' ');
                    }
                    g_string_append(out, *p);
                }
                g_strfreev(pieces);
                g_free(name);
                name = g_string_free(out, FALSE);
            }
            break;
        }
        g_strfreev(lines);
        g_free(text);
    }
    return name;
}

static gboolean is_cpu_chip(const char *name) {
    static const char *const chips[] = {"coretemp", "k10temp", "zenpower", "cpu_thermal"};
    for (size_t i = 0; i < G_N_ELEMENTS(chips); i++) {
        if (g_strcmp0(name, chips[i]) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

static gboolean has_preferred_label(const char *label) {
    static const char *const prefixes[] = {"package", "tctl", "tdie"};
    char *low = g_ascii_strdown(label, -1);
    gboolean found = FALSE;
    for (size_t i = 0; i < G_N_ELEMENTS(prefixes) && !found; i++) {
        found = g_str_has_prefix(low, prefixes[i]);
    }
    g_free(low);
    return found;
}

static PcTemp *copy_temp(const PcTemp *t) {
    return pc_temp_new(t->label, t->value, t->crit);
}

PcTemp *pc_cpu_temp(const char *sys_root) {
    char *hwmon_dir = g_build_filename(sys_root, "class", "hwmon", NULL);
    GPtrArray *dirs = pc_glob_dir(hwmon_dir, "hwmon*");
    g_free(hwmon_dir);
    PcTemp *result = NULL;
    for (guint i = 0; i < dirs->len && result == NULL; i++) {
        const char *dir = g_ptr_array_index(dirs, i);
        char *name_path = g_build_filename(dir, "name", NULL);
        char *name = pc_read_text(name_path);
        g_free(name_path);
        if (name != NULL && is_cpu_chip(name)) {
            GPtrArray *temps = pc_hwmon_temps(dir);
            if (temps->len > 0) {
                const PcTemp *pick = g_ptr_array_index(temps, 0);
                for (guint k = 0; k < temps->len; k++) {
                    const PcTemp *t = g_ptr_array_index(temps, k);
                    if (has_preferred_label(t->label)) {
                        pick = t;
                        break;
                    }
                }
                result = copy_temp(pick);
            }
            g_ptr_array_unref(temps);
        }
        g_free(name);
    }
    g_ptr_array_unref(dirs);
    if (result != NULL) {
        return result;
    }

    char *thermal_dir = g_build_filename(sys_root, "class", "thermal", NULL);
    GPtrArray *zones = pc_glob_dir(thermal_dir, "thermal_zone*");
    g_free(thermal_dir);
    for (guint i = 0; i < zones->len && result == NULL; i++) {
        const char *zone = g_ptr_array_index(zones, i);
        char *type_path = g_build_filename(zone, "type", NULL);
        char *type = pc_read_text(type_path);
        g_free(type_path);
        if (g_strcmp0(type, "x86_pkg_temp") == 0 || g_strcmp0(type, "cpu-thermal") == 0 ||
            g_strcmp0(type, "cpu_thermal") == 0) {
            char *temp_path = g_build_filename(zone, "temp", NULL);
            gint64 milli;
            if (pc_read_int(temp_path, &milli)) {
                result = pc_temp_new("CPU", milli / 1000.0, pc_none());
            }
            g_free(temp_path);
        }
        g_free(type);
    }
    g_ptr_array_unref(zones);
    return result;
}

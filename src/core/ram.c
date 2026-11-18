// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Parses /proc/meminfo into used and total bytes.

#include "core/ram.h"

#include <string.h>

#include "core/fsread.h"

// The kibibyte count after "<key>:" in the line, in bytes. FALSE when the line has another key.
static gboolean line_value(const char *line, const char *key, double *bytes) {
    size_t len = strlen(key);
    if (strncmp(line, key, len) != 0 || line[len] != ':') {
        return FALSE;
    }
    const char *p = line + len + 1;
    while (g_ascii_isspace(*p)) {
        p++;
    }
    if (!g_ascii_isdigit(*p)) {
        return FALSE;
    }
    gint64 kib = 0;
    while (g_ascii_isdigit(*p)) {
        kib = kib * 10 + (*p - '0');
        p++;
    }
    *bytes = (double)kib * 1024.0;
    return TRUE;
}

PcRam pc_parse_meminfo(const char *text) {
    PcRam ram = {FALSE, 0.0, 0.0};
    if (text == NULL) {
        return ram;
    }
    double total = 0, available = 0, free_mem = 0;
    gboolean have_available = FALSE, have_free = FALSE;
    char **lines = g_strsplit(text, "\n", -1);
    for (char **line = lines; *line != NULL; line++) {
        double v;
        if (line_value(*line, "MemTotal", &v)) {
            total = v;
        } else if (line_value(*line, "MemAvailable", &v)) {
            available = v;
            have_available = TRUE;
        } else if (line_value(*line, "MemFree", &v)) {
            free_mem = v;
            have_free = TRUE;
        }
    }
    g_strfreev(lines);
    if (total <= 0.0) {
        return ram;
    }
    double avail = have_available ? available : (have_free ? free_mem : 0.0);
    ram.present = TRUE;
    ram.total = total;
    ram.used = total - avail;
    return ram;
}

PcRam pc_read_ram(const char *proc_root) {
    char *path = g_build_filename(proc_root, "meminfo", NULL);
    char *text = pc_read_text(path);
    g_free(path);
    PcRam ram = pc_parse_meminfo(text);
    g_free(text);
    return ram;
}

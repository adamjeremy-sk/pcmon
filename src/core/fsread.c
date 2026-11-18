// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Reads sysfs and procfs files, parses numbers, lists and orders directory entries.

#include "core/fsread.h"

#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

char *pc_read_text(const char *path) {
    char *contents = NULL;
    if (!g_file_get_contents(path, &contents, NULL, NULL)) {
        return NULL;
    }
    g_strstrip(contents);
    return contents;
}

gboolean pc_parse_int(const char *text, gint64 *out) {
    if (text == NULL) {
        return FALSE;
    }
    char *copy = g_strstrip(g_strdup(text));
    char *end = NULL;
    errno = 0;
    gint64 value = g_ascii_strtoll(copy, &end, 10);
    gboolean ok = *copy != '\0' && *end == '\0' && errno == 0;
    g_free(copy);
    if (ok) {
        *out = value;
    }
    return ok;
}

gboolean pc_parse_double(const char *text, double *out) {
    if (text == NULL) {
        return FALSE;
    }
    char *copy = g_strstrip(g_strdup(text));
    char *end = NULL;
    double value = g_ascii_strtod(copy, &end);
    gboolean ok = *copy != '\0' && *end == '\0' && isfinite(value);
    g_free(copy);
    if (ok) {
        *out = value;
    }
    return ok;
}

gboolean pc_read_int(const char *path, gint64 *out) {
    char *text = pc_read_text(path);
    gboolean ok = pc_parse_int(text, out);
    g_free(text);
    return ok;
}

int pc_tail_number(const char *text) {
    size_t end = strlen(text);
    while (end > 0 && !g_ascii_isdigit(text[end - 1])) {
        end--;
    }
    size_t start = end;
    while (start > 0 && g_ascii_isdigit(text[start - 1])) {
        start--;
    }
    gint64 value = 0;
    for (size_t i = start; i < end && value < INT_MAX / 10; i++) {
        value = value * 10 + (text[i] - '0');
    }
    return (int)value;
}

static int compare_names(gconstpointer a, gconstpointer b) {
    const char *x = *(const char *const *)a;
    const char *y = *(const char *const *)b;
    int nx = pc_tail_number(x);
    int ny = pc_tail_number(y);
    if (nx != ny) {
        return nx < ny ? -1 : 1;
    }
    return strcmp(x, y);
}

GPtrArray *pc_glob_dir(const char *dir, const char *pattern) {
    GPtrArray *names = g_ptr_array_new_with_free_func(g_free);
    GDir *handle = g_dir_open(dir, 0, NULL);
    if (handle == NULL) {
        return names;
    }
    const char *name;
    while ((name = g_dir_read_name(handle)) != NULL) {
        if (g_pattern_match_simple(pattern, name)) {
            g_ptr_array_add(names, g_strdup(name));
        }
    }
    g_dir_close(handle);
    g_ptr_array_sort(names, compare_names);
    // Names become full paths after sorting, so the order comes from the entry names only.
    for (guint i = 0; i < names->len; i++) {
        char *full = g_build_filename(dir, g_ptr_array_index(names, i), NULL);
        g_free(g_ptr_array_index(names, i));
        g_ptr_array_index(names, i) = full;
    }
    return names;
}

char *pc_realpath(const char *path) {
    char *resolved = realpath(path, NULL);
    if (resolved == NULL) {
        return g_strdup(path);
    }
    char *copy = g_strdup(resolved);
    free(resolved);
    return copy;
}

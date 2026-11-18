// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Display text and fractions for temperatures and fans.

#include "core/format.h"

#include <math.h>
#include <string.h>

char *pc_temp_name(const char *label) {
    char *low = g_ascii_strdown(label, -1);
    const char *pretty = NULL;
    if (strcmp(low, "edge") == 0) {
        pretty = "Edge";
    } else if (strcmp(low, "junction") == 0) {
        pretty = "Junction";
    } else if (strcmp(low, "mem") == 0) {
        pretty = "Memory";
    }
    char *result;
    if (pretty != NULL) {
        result = g_strdup_printf("%s temp", pretty);
    } else if (g_str_has_prefix(low, "temp") && low[4] != '\0' &&
               strspn(low + 4, "0123456789") == strlen(low + 4)) {
        result = g_strdup("Temperature");
    } else {
        result = g_strdup(label);
    }
    g_free(low);
    return result;
}

double pc_temp_frac(const PcTemp *temp) {
    double limit = (temp->crit.set && temp->crit.value != 0.0) ? temp->crit.value : 100.0;
    return CLAMP(temp->value / limit, 0.0, 1.0);
}

// 1234567 -> "1,234,567" (always a comma, whatever the locale).
static char *with_thousands(const char *digits) {
    GString *out = g_string_new(NULL);
    gboolean negative = digits[0] == '-';
    const char *p = negative ? digits + 1 : digits;
    size_t len = strlen(p);
    if (negative) {
        g_string_append_c(out, '-');
    }
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && (len - i) % 3 == 0) {
            g_string_append_c(out, ',');
        }
        g_string_append_c(out, p[i]);
    }
    return g_string_free(out, FALSE);
}

char *pc_format_fan(const PcFan *fan) {
    char plain[64];
    g_snprintf(plain, sizeof plain, "%.0f", fan->value);
    if (fan->percent) {
        return g_strdup_printf("%s%%", plain);
    }
    char *grouped = with_thousands(plain);
    char *text = g_strdup_printf("%s RPM", grouped);
    g_free(grouped);
    return text;
}

char *pc_fan_name(const PcFan *fan, guint fan_count) {
    if (fan_count == 1) {
        return g_strdup("Fan");
    }
    if (g_ascii_strncasecmp(fan->label, "fan", 3) == 0) {
        const char *digits = fan->label + 3;
        if (digits[0] != '\0' && strspn(digits, "0123456789") == strlen(digits)) {
            return g_strdup_printf("Fan %s", digits);
        }
    }
    return g_strdup(fan->label);
}

char *pc_fan_id(const PcFan *fan) {
    const char *end = fan->label + strlen(fan->label);
    while (end > fan->label && g_ascii_isspace(end[-1])) {
        end--;
    }
    const char *start = end;
    while (start > fan->label && !g_ascii_isspace(start[-1])) {
        start--;
    }
    return g_strndup(start, (gsize)(end - start));
}

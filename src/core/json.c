// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// A small JSON writer for snapshots (no JSON library needed).

#include "core/json.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static void indent(GString *out, int depth) {
    for (int i = 0; i < depth; i++) {
        g_string_append(out, "  ");
    }
}

static void put_string(GString *out, const char *text) {
    if (text == NULL) {
        g_string_append(out, "null");
        return;
    }
    g_string_append_c(out, '"');
    for (const unsigned char *p = (const unsigned char *)text; *p != '\0'; p++) {
        switch (*p) {
        case '"':
            g_string_append(out, "\\\"");
            break;
        case '\\':
            g_string_append(out, "\\\\");
            break;
        case '\n':
            g_string_append(out, "\\n");
            break;
        case '\t':
            g_string_append(out, "\\t");
            break;
        default:
            if (*p < 0x20) {
                g_string_append_printf(out, "\\u%04x", *p);
            } else {
                g_string_append_c(out, (char)*p);
            }
        }
    }
    g_string_append_c(out, '"');
}

// The shortest text that reads back as the same double.
static void put_number(GString *out, double v) {
    char buf[40];
    snprintf(buf, sizeof buf, "%.15g", v);
    if (strtod(buf, NULL) != v) {
        snprintf(buf, sizeof buf, "%.17g", v);
    }
    g_string_append(out, buf);
}

static void put_opt(GString *out, PcOpt o) {
    if (o.set) {
        put_number(out, o.value);
    } else {
        g_string_append(out, "null");
    }
}

static void put_temp(GString *out, const PcTemp *t, int depth) {
    g_string_append(out, "{\n");
    indent(out, depth + 1);
    g_string_append(out, "\"label\": ");
    put_string(out, t->label);
    g_string_append(out, ",\n");
    indent(out, depth + 1);
    g_string_append(out, "\"value\": ");
    put_number(out, t->value);
    g_string_append(out, ",\n");
    indent(out, depth + 1);
    g_string_append(out, "\"crit\": ");
    put_opt(out, t->crit);
    g_string_append_c(out, '\n');
    indent(out, depth);
    g_string_append_c(out, '}');
}

static void put_fan(GString *out, const PcFan *f, int depth) {
    g_string_append(out, "{\n");
    indent(out, depth + 1);
    g_string_append(out, "\"label\": ");
    put_string(out, f->label);
    g_string_append(out, ",\n");
    indent(out, depth + 1);
    g_string_append(out, "\"value\": ");
    put_number(out, f->value);
    g_string_append(out, ",\n");
    indent(out, depth + 1);
    g_string_append(out, "\"unit\": ");
    put_string(out, f->percent ? "%" : "RPM");
    g_string_append_c(out, '\n');
    indent(out, depth);
    g_string_append_c(out, '}');
}

static void put_temps(GString *out, const GPtrArray *temps, int depth) {
    if (temps->len == 0) {
        g_string_append(out, "[]");
        return;
    }
    g_string_append(out, "[\n");
    for (guint i = 0; i < temps->len; i++) {
        indent(out, depth + 1);
        put_temp(out, g_ptr_array_index(temps, i), depth + 1);
        g_string_append(out, i + 1 < temps->len ? ",\n" : "\n");
    }
    indent(out, depth);
    g_string_append_c(out, ']');
}

static void put_fans(GString *out, const GPtrArray *fans, int depth) {
    if (fans->len == 0) {
        g_string_append(out, "[]");
        return;
    }
    g_string_append(out, "[\n");
    for (guint i = 0; i < fans->len; i++) {
        indent(out, depth + 1);
        put_fan(out, g_ptr_array_index(fans, i), depth + 1);
        g_string_append(out, i + 1 < fans->len ? ",\n" : "\n");
    }
    indent(out, depth);
    g_string_append_c(out, ']');
}

static void put_gpu(GString *out, const PcGpu *g, int depth) {
    g_string_append(out, "{\n");
    indent(out, depth + 1);
    g_string_append(out, "\"name\": ");
    put_string(out, g->name);
    g_string_append(out, ",\n");
    indent(out, depth + 1);
    g_string_append(out, "\"vendor\": ");
    put_string(out, g->vendor);
    g_string_append(out, ",\n");
    indent(out, depth + 1);
    g_string_append(out, "\"slot\": ");
    put_string(out, g->slot);
    g_string_append(out, ",\n");
    indent(out, depth + 1);
    g_string_append(out, "\"temps\": ");
    put_temps(out, g->temps, depth + 1);
    g_string_append(out, ",\n");
    indent(out, depth + 1);
    g_string_append(out, "\"fans\": ");
    put_fans(out, g->fans, depth + 1);
    g_string_append(out, ",\n");
    indent(out, depth + 1);
    g_string_append(out, "\"vram_used\": ");
    put_opt(out, g->vram_used);
    g_string_append(out, ",\n");
    indent(out, depth + 1);
    g_string_append(out, "\"vram_total\": ");
    put_opt(out, g->vram_total);
    g_string_append(out, ",\n");
    indent(out, depth + 1);
    g_string_append(out, "\"load\": ");
    put_opt(out, g->load);
    g_string_append(out, ",\n");
    indent(out, depth + 1);
    g_string_append(out, "\"shared_memory\": ");
    g_string_append(out, g->shared_memory ? "true" : "false");
    g_string_append_c(out, '\n');
    indent(out, depth);
    g_string_append_c(out, '}');
}

char *pc_snapshot_to_json(const PcSnapshot *snap) {
    GString *out = g_string_new("{\n  \"cpu\": {\n    \"name\": ");
    put_string(out, snap->cpu.name);
    g_string_append(out, ",\n    \"temp\": ");
    if (snap->cpu.has_temp) {
        put_temp(out, &snap->cpu.temp, 2);
    } else {
        g_string_append(out, "null");
    }
    g_string_append(out, ",\n    \"load\": ");
    put_opt(out, snap->cpu.load);
    g_string_append(out, "\n  },\n  \"gpus\": ");
    if (snap->gpus->len == 0) {
        g_string_append(out, "[]");
    } else {
        g_string_append(out, "[\n");
        for (guint i = 0; i < snap->gpus->len; i++) {
            indent(out, 2);
            put_gpu(out, g_ptr_array_index(snap->gpus, i), 2);
            g_string_append(out, i + 1 < snap->gpus->len ? ",\n" : "\n");
        }
        g_string_append(out, "  ]");
    }
    g_string_append(out, ",\n  \"fans\": ");
    put_fans(out, snap->fans, 1);
    g_string_append(out, ",\n  \"ram\": ");
    if (snap->ram.present) {
        g_string_append(out, "{\n    \"used\": ");
        put_number(out, snap->ram.used);
        g_string_append(out, ",\n    \"total\": ");
        put_number(out, snap->ram.total);
        g_string_append(out, "\n  }");
    } else {
        g_string_append(out, "null");
    }
    g_string_append(out, "\n}\n");
    return g_string_free(out, FALSE);
}

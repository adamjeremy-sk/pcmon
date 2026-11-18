// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Reads NVIDIA GPUs through nvidia-smi and caches the answer briefly.

#include "core/nvidia.h"

#include <string.h>

#include "core/fsread.h"
#include "core/proc_run.h"

#define SMI_CACHE_US (2 * G_USEC_PER_SEC)
#define MIB 1048576.0

static void nvidia_gpu_free(PcNvidiaGpu *gpu) {
    g_free(gpu->name);
    g_free(gpu);
}

static PcOpt opt_number(const char *text) {
    double v;
    return pc_parse_double(text, &v) ? pc_some(v) : pc_none();
}

static PcOpt opt_mib(const char *text) {
    PcOpt o = opt_number(text);
    if (o.set) {
        o.value = (double)(gint64)(o.value * MIB); // whole bytes, as int() does
    }
    return o;
}

GHashTable *pc_parse_nvidia_smi(const char *text) {
    GHashTable *table =
        g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)nvidia_gpu_free);
    if (text == NULL) {
        return table;
    }
    char *copy = g_strstrip(g_strdup(text));
    char **lines = g_strsplit(copy, "\n", -1);
    g_free(copy);
    for (char **line = lines; *line != NULL; line++) {
        char **parts = g_strsplit(*line, ",", -1);
        guint count = g_strv_length(parts);
        if (count >= 6) {
            for (guint i = 0; i < count; i++) {
                g_strstrip(parts[i]);
            }
            char *bus = g_ascii_strdown(parts[0], -1);
            size_t len = strlen(bus);
            // "00000000:01:00.0" -> "0000:01:00.0": the last 12 characters
            char *key = g_strdup(len > 12 ? bus + (len - 12) : bus);
            g_free(bus);
            PcNvidiaGpu *gpu = g_new0(PcNvidiaGpu, 1);
            gpu->name = g_strdup(parts[1]);
            gpu->temp = opt_number(parts[2]);
            gpu->used = opt_mib(parts[3]);
            gpu->total = opt_mib(parts[4]);
            gpu->fan_pct = opt_number(parts[5]);
            gpu->load = count > 6 ? opt_number(parts[6]) : pc_none();
            g_hash_table_replace(table, key, gpu);
        }
        g_strfreev(parts);
    }
    g_strfreev(lines);
    return table;
}

struct PcNvidiaCache {
    gboolean enabled;
    gint64 taken_at;
    GHashTable *table;
};

PcNvidiaCache *pc_nvidia_cache_new(gboolean enabled) {
    PcNvidiaCache *c = g_new0(PcNvidiaCache, 1);
    c->enabled = enabled;
    c->table = pc_parse_nvidia_smi(NULL);
    return c;
}

const GHashTable *pc_nvidia_cache_get(PcNvidiaCache *cache) {
    if (!cache->enabled) {
        return cache->table;
    }
    gint64 now = g_get_monotonic_time();
    if (cache->taken_at != 0 && now - cache->taken_at < SMI_CACHE_US) {
        return cache->table;
    }
    cache->taken_at = now;
    g_hash_table_unref(cache->table);
    cache->table = NULL;
    char *smi = g_find_program_in_path("nvidia-smi");
    if (smi != NULL) {
        char *argv[] = {smi, "--query-gpu=" PC_SMI_QUERY, "--format=csv,noheader,nounits", NULL};
        char *out = pc_run_capture(argv, 2000);
        cache->table = pc_parse_nvidia_smi(out);
        g_free(out);
        g_free(smi);
    }
    if (cache->table == NULL) {
        cache->table = pc_parse_nvidia_smi(NULL);
    }
    return cache->table;
}

void pc_nvidia_cache_free(PcNvidiaCache *cache) {
    if (cache == NULL) {
        return;
    }
    g_hash_table_unref(cache->table);
    g_free(cache);
}

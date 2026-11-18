// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Allocation and release of the snapshot types.

#include "core/types.h"

PcTemp *pc_temp_new(const char *label, double value, PcOpt crit) {
    PcTemp *t = g_new0(PcTemp, 1);
    t->label = g_strdup(label);
    t->value = value;
    t->crit = crit;
    return t;
}

PcFan *pc_fan_new(const char *label, double value, gboolean percent) {
    PcFan *f = g_new0(PcFan, 1);
    f->label = g_strdup(label);
    f->value = value;
    f->percent = percent;
    return f;
}

void pc_temp_free(PcTemp *temp) {
    if (temp == NULL) {
        return;
    }
    g_free(temp->label);
    g_free(temp);
}

void pc_fan_free(PcFan *fan) {
    if (fan == NULL) {
        return;
    }
    g_free(fan->label);
    g_free(fan);
}

GPtrArray *pc_temp_array_new(void) {
    return g_ptr_array_new_with_free_func((GDestroyNotify)pc_temp_free);
}

GPtrArray *pc_fan_array_new(void) {
    return g_ptr_array_new_with_free_func((GDestroyNotify)pc_fan_free);
}

void pc_gpu_free(PcGpu *gpu) {
    if (gpu == NULL) {
        return;
    }
    g_free(gpu->name);
    g_free(gpu->vendor);
    g_free(gpu->slot);
    if (gpu->temps != NULL) {
        g_ptr_array_unref(gpu->temps);
    }
    if (gpu->fans != NULL) {
        g_ptr_array_unref(gpu->fans);
    }
    g_free(gpu);
}

PcSnapshot *pc_snapshot_new(void) {
    PcSnapshot *s = g_new0(PcSnapshot, 1);
    s->gpus = g_ptr_array_new_with_free_func((GDestroyNotify)pc_gpu_free);
    s->fans = pc_fan_array_new();
    return s;
}

void pc_snapshot_free(PcSnapshot *snap) {
    if (snap == NULL) {
        return;
    }
    g_free(snap->cpu.name);
    if (snap->cpu.has_temp) {
        g_free(snap->cpu.temp.label);
    }
    g_ptr_array_unref(snap->gpus);
    g_ptr_array_unref(snap->fans);
    g_free(snap);
}

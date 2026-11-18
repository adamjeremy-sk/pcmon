// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Combines the CPU, GPU, fan and RAM readers into one snapshot.

#include "core/sensors.h"

#include "core/cpu.h"
#include "core/fsread.h"
#include "core/gpu.h"
#include "core/hwmon.h"
#include "core/ram.h"

struct PcSensors {
    char *sys;
    char *proc;
    PcGpuReader *gpus;
    char *cpu_name; // read once
    gboolean have_prev;
    PcCpuTimes prev_cpu;
};

PcSensors *pc_sensors_new(const char *root) {
    PcSensors *s = g_new0(PcSensors, 1);
    s->sys = g_build_filename(root, "sys", NULL);
    s->proc = g_build_filename(root, "proc", NULL);
    // lspci and nvidia-smi describe the real machine, so they run only when root is "/".
    s->gpus = pc_gpu_reader_new(s->sys, g_strcmp0(root, "/") == 0);
    return s;
}

void pc_sensors_free(PcSensors *sensors) {
    if (sensors == NULL) {
        return;
    }
    g_free(sensors->sys);
    g_free(sensors->proc);
    g_free(sensors->cpu_name);
    pc_gpu_reader_free(sensors->gpus);
    g_free(sensors);
}

static gboolean read_cpu_times(const PcSensors *s, PcCpuTimes *out) {
    char *path = g_build_filename(s->proc, "stat", NULL);
    char *text = pc_read_text(path);
    g_free(path);
    gboolean ok = pc_parse_cpu_times(text, out);
    g_free(text);
    return ok;
}

// CPU busy percent since the previous call (the first call measures over 0.25 s).
static PcOpt cpu_load(PcSensors *s) {
    PcCpuTimes cur;
    if (!read_cpu_times(s, &cur)) {
        return pc_none();
    }
    PcCpuTimes prev = s->prev_cpu;
    if (!s->have_prev) {
        g_usleep(250 * 1000);
        prev = cur;
        PcCpuTimes again;
        if (read_cpu_times(s, &again)) {
            cur = again;
        }
    }
    s->prev_cpu = cur;
    s->have_prev = TRUE;
    double load;
    return pc_cpu_load(&prev, &cur, &load) ? pc_some(load) : pc_none();
}

// Every hwmon fan that does not belong to a GPU, including 0 RPM ones (the window decides
// what to hide).
static GPtrArray *system_fans(const PcSensors *s, const GHashTable *gpu_devs) {
    GPtrArray *fans = pc_fan_array_new();
    char *hwmon_root = g_build_filename(s->sys, "class", "hwmon", NULL);
    GPtrArray *dirs = pc_glob_dir(hwmon_root, "hwmon*");
    g_free(hwmon_root);
    for (guint i = 0; i < dirs->len; i++) {
        const char *dir = g_ptr_array_index(dirs, i);
        char *device = g_build_filename(dir, "device", NULL);
        char *real = pc_realpath(device);
        g_free(device);
        gboolean is_gpu = g_hash_table_contains((GHashTable *)gpu_devs, real);
        g_free(real);
        if (is_gpu) {
            continue;
        }
        char *name_path = g_build_filename(dir, "name", NULL);
        char *chip = pc_read_text(name_path);
        g_free(name_path);
        if (chip == NULL || chip[0] == '\0') {
            g_free(chip);
            chip = g_path_get_basename(dir);
        }
        GPtrArray *found = pc_hwmon_fans(dir, chip);
        g_free(chip);
        for (guint k = 0; k < found->len; k++) {
            g_ptr_array_add(fans, g_steal_pointer(&g_ptr_array_index(found, k)));
        }
        g_ptr_array_unref(found);
    }
    g_ptr_array_unref(dirs);
    return fans;
}

PcSnapshot *pc_sensors_collect(PcSensors *sensors) {
    PcSnapshot *snap = pc_snapshot_new();
    GHashTable *devs = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    g_ptr_array_unref(snap->gpus);
    snap->gpus = pc_gpu_reader_read(sensors->gpus, devs);

    if (sensors->cpu_name == NULL) {
        sensors->cpu_name = pc_cpu_name(sensors->proc);
    }
    snap->cpu.name = g_strdup(sensors->cpu_name);
    PcTemp *temp = pc_cpu_temp(sensors->sys);
    if (temp != NULL) {
        snap->cpu.has_temp = TRUE;
        snap->cpu.temp = *temp;
        g_free(temp); // the label now belongs to the snapshot
    }
    snap->cpu.load = cpu_load(sensors);

    g_ptr_array_unref(snap->fans);
    snap->fans = system_fans(sensors, devs);
    snap->ram = pc_read_ram(sensors->proc);
    g_hash_table_unref(devs);
    return snap;
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// CPU readings: busy time from /proc/stat, model name, and the package temperature.

#pragma once

#include "core/types.h"

// Jiffy counters from the first line of /proc/stat.
typedef struct {
    gint64 total;
    gint64 idle; // idle plus iowait
} PcCpuTimes;

// Parses the "cpu " line of /proc/stat text. FALSE when it is missing or malformed.
gboolean pc_parse_cpu_times(const char *text, PcCpuTimes *out);

// Percent of time the CPU was busy between two samples, clamped to 0..100.
// FALSE when no time passed between them.
gboolean pc_cpu_load(const PcCpuTimes *prev, const PcCpuTimes *cur, double *out);

// The "model name" of /proc/cpuinfo under proc_root with white space collapsed ("" if absent).
char *pc_cpu_name(const char *proc_root);

// The whole-CPU temperature: a coretemp, k10temp, zenpower or cpu_thermal hwmon chip first
// (preferring its package / Tctl / Tdie reading), then a CPU thermal zone. NULL when none.
PcTemp *pc_cpu_temp(const char *sys_root);

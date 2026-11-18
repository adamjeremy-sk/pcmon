// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// The plain data types of one sensor snapshot, and how to free them.

#pragma once

#include <glib.h>

// A number that may be missing (the Python original used None).
typedef struct {
    gboolean set;
    double value;
} PcOpt;

static inline PcOpt pc_some(double value) {
    PcOpt o = {TRUE, value};
    return o;
}

static inline PcOpt pc_none(void) {
    PcOpt o = {FALSE, 0.0};
    return o;
}

// One temperature in degrees C; crit is the sensor's own critical or maximum limit.
typedef struct {
    char *label;
    double value;
    PcOpt crit;
} PcTemp;

// One fan: RPM, or a percentage when percent is set (NVIDIA reports a percentage).
typedef struct {
    char *label;
    double value;
    gboolean percent;
} PcFan;

typedef struct {
    char *name;
    char *vendor;
    char *slot;             // PCI slot such as "0000:03:00.0", or NULL
    GPtrArray *temps;       // PcTemp*
    GPtrArray *fans;        // PcFan*
    PcOpt vram_used;        // bytes
    PcOpt vram_total;       // bytes
    PcOpt load;             // percent
    gboolean shared_memory; // integrated graphics that use system RAM
} PcGpu;

typedef struct {
    char *name;
    gboolean has_temp;
    PcTemp temp; // label owned by the snapshot when has_temp is set
    PcOpt load;  // percent
} PcCpu;

typedef struct {
    gboolean present;
    double used;  // bytes
    double total; // bytes
} PcRam;

typedef struct {
    PcCpu cpu;
    GPtrArray *gpus; // PcGpu*
    GPtrArray *fans; // PcFan*, every fan that does not belong to a GPU
    PcRam ram;
} PcSnapshot;

PcTemp *pc_temp_new(const char *label, double value, PcOpt crit);
PcFan *pc_fan_new(const char *label, double value, gboolean percent);
GPtrArray *pc_temp_array_new(void);
GPtrArray *pc_fan_array_new(void);
void pc_temp_free(PcTemp *temp);
void pc_fan_free(PcFan *fan);
void pc_gpu_free(PcGpu *gpu);
PcSnapshot *pc_snapshot_new(void);
void pc_snapshot_free(PcSnapshot *snap);

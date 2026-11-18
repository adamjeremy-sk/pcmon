// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// NVIDIA cards: the proprietary driver exposes nothing in sysfs, so nvidia-smi is asked.

#pragma once

#include "core/types.h"

typedef struct {
    char *name;
    PcOpt temp;  // degrees C
    PcOpt used;  // VRAM bytes
    PcOpt total; // VRAM bytes
    PcOpt fan_pct;
    PcOpt load; // percent
} PcNvidiaGpu;

// The columns that pc_parse_nvidia_smi expects, in this order.
#define PC_SMI_QUERY                                                                               \
    "pci.bus_id,name,temperature.gpu,memory.used,memory.total,fan.speed,utilization.gpu"

// Parses `nvidia-smi --query-gpu=PC_SMI_QUERY --format=csv,noheader,nounits` output into a table
// keyed by PCI slot ("0000:01:00.0", lower case) holding PcNvidiaGpu*.
GHashTable *pc_parse_nvidia_smi(const char *text);

// Asks nvidia-smi (at most once every two seconds); the table is owned by the cache and stays
// valid until the next call. Empty when nvidia-smi is not installed, or when the cache was made
// with enabled FALSE (tests run on a fake tree and must not ask the real machine).
typedef struct PcNvidiaCache PcNvidiaCache;
PcNvidiaCache *pc_nvidia_cache_new(gboolean enabled);
const GHashTable *pc_nvidia_cache_get(PcNvidiaCache *cache);
void pc_nvidia_cache_free(PcNvidiaCache *cache);

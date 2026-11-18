// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Finds every GPU under /sys/class/drm and reads its load, temperatures, VRAM and fans.

#pragma once

#include "core/types.h"

// Turns one `lspci -mm` line into a short GPU name, for example
// "Radeon RX 7900 XTX" (prefixed with the vendor when the name does not contain it).
// NULL when the line cannot be understood.
char *pc_pretty_gpu_name(const char *lspci_line, const char *vendor_name);

typedef struct PcGpuReader PcGpuReader;

// sys_root is "/sys" or a fake tree for tests. run_helpers lets the reader start lspci and
// nvidia-smi; it is FALSE for a fake tree, because those programs describe the real machine.
PcGpuReader *pc_gpu_reader_new(const char *sys_root, gboolean run_helpers);
void pc_gpu_reader_free(PcGpuReader *reader);

// Returns PcGpu* elements. When devs is not NULL it receives (as g_free'd strings in a set)
// the resolved device paths of the GPUs, so their hwmon fans can be left out of the system fans.
GPtrArray *pc_gpu_reader_read(PcGpuReader *reader, GHashTable *devs);

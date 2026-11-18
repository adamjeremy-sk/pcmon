// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// System memory use from /proc/meminfo.

#pragma once

#include "core/types.h"

// Parses /proc/meminfo text. used = MemTotal - MemAvailable (or MemFree when the kernel has
// no MemAvailable). present is FALSE when MemTotal is missing.
PcRam pc_parse_meminfo(const char *text);

// Reads meminfo under proc_root.
PcRam pc_read_ram(const char *proc_root);

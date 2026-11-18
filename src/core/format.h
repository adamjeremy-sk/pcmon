// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Text and ratios the window shows: temperature names, fan names, fan speeds, bar fractions.

#pragma once

#include "core/types.h"

// "edge" -> "Edge temp", "junction" -> "Junction temp", "mem" -> "Memory temp",
// "temp3" -> "Temperature", anything else unchanged. Caller frees.
char *pc_temp_name(const char *label);

// How close a temperature is to its critical limit, 0..1 (100 C when the sensor has none).
double pc_temp_frac(const PcTemp *temp);

// "1,234 RPM" or "45%". Caller frees.
char *pc_format_fan(const PcFan *fan);

// "Fan" for a lone fan, "fan3" -> "Fan 3" (any case), otherwise the label itself. Caller frees.
char *pc_fan_name(const PcFan *fan, guint fan_count);

// The key fan names are stored under: the last word of the label ("nct6687 fan1" -> "fan1").
char *pc_fan_id(const PcFan *fan);

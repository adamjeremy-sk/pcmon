// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Temperatures and fans of one hwmon directory.

#pragma once

#include "core/types.h"

// Every temp*_input of dir in degrees C, with its label and critical or maximum limit.
// Returns PcTemp* elements.
GPtrArray *pc_hwmon_temps(const char *dir);

// Every fan*_input of dir in RPM. prefix (may be "" or NULL) is put before each label.
// Returns PcFan* elements.
GPtrArray *pc_hwmon_fans(const char *dir, const char *prefix);

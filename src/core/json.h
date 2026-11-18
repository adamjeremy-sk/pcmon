// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Writes a snapshot as JSON, for the pcmon-sensors tool and for checking against the Python.

#pragma once

#include "core/types.h"

// Indented JSON (two spaces) with the same field names the Python version printed.
char *pc_snapshot_to_json(const PcSnapshot *snap);

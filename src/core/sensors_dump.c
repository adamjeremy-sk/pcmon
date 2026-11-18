// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// The pcmon-sensors tool: prints every sensor that was found, as JSON.

#include <stdio.h>

#include "core/json.h"
#include "core/sensors.h"

// Usage: pcmon-sensors [ROOT]   (ROOT is a fake tree holding sys/ and proc/, for testing)
int main(int argc, char **argv) {
    PcSensors *sensors = pc_sensors_new(argc > 1 ? argv[1] : "/");
    PcSnapshot *snap = pc_sensors_collect(sensors);
    char *json = pc_snapshot_to_json(snap);
    fputs(json, stdout);
    g_free(json);
    pc_snapshot_free(snap);
    pc_sensors_free(sensors);
    return 0;
}

# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
# The sensor library (GLib only, no GTK) and the pcmon-sensors command line tool.
add_library(pcmon_core STATIC
    types.c fsread.c hwmon.c cpu.c ram.c proc_run.c nvidia.c gpu.c sensors.c json.c
    settings.c format.c)
target_include_directories(pcmon_core PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(pcmon_core PUBLIC PkgConfig::GLIB2 PRIVATE pcmon_warnings)

add_executable(pcmon-sensors sensors_dump.c)
target_link_libraries(pcmon-sensors PRIVATE pcmon_core pcmon_warnings)

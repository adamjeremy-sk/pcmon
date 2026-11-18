# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
# Top-level build: C11, GLib and GTK 4, warnings as errors.
cmake_minimum_required(VERSION 3.28)
project(pcmon VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON) # gnu11: the POSIX calls (realpath, poll, kill) need it
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(PCMON_BUILD_UI "Build the GTK 4 window (turn off to build only the sensor code and tests)" ON)

add_library(pcmon_warnings INTERFACE)
target_compile_options(pcmon_warnings INTERFACE -Wall -Wextra -Werror -Wno-cast-function-type)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB2 REQUIRED IMPORTED_TARGET glib-2.0)

enable_testing()

add_subdirectory(src/core)
if(PCMON_BUILD_UI)
    add_subdirectory(src/ui)
endif()
add_subdirectory(tests)

include(GNUInstallDirs)
install(TARGETS pcmon-sensors RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
if(PCMON_BUILD_UI)
    install(TARGETS pcmon RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES data/io.github.adamjeremy_sk.Pcmon.desktop
            DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES data/pcmon.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
    install(FILES data/pcmon.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/512x512/apps)
endif()

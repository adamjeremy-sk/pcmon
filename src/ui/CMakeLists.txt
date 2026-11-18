# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
# The GTK 4 program. Needs libgtk-4-dev; libx11-dev is optional (always-on-top on X11).
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)
pkg_check_modules(X11 IMPORTED_TARGET x11)

add_executable(pcmon
    main.c window.c options.c poller.c section.c row.c label.c style.c keep_above.c)
target_include_directories(pcmon PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(pcmon PRIVATE pcmon_core PkgConfig::GTK4 pcmon_warnings)
if(X11_FOUND)
    target_compile_definitions(pcmon PRIVATE PCMON_HAVE_X11)
    target_link_libraries(pcmon PRIVATE PkgConfig::X11)
else()
    message(STATUS "libx11 not found: 'Always on top' will be disabled")
endif()

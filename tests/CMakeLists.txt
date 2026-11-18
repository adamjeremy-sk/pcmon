# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
# Unit tests (GLib's test framework) and the repository checks run by ctest.
add_library(pcmon_test_support STATIC fixture.c)
target_include_directories(pcmon_test_support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pcmon_test_support PUBLIC pcmon_core PRIVATE pcmon_warnings)

foreach(name IN ITEMS test_fsread test_hwmon test_cpu test_ram test_gpu test_sensors
                      test_settings test_format)
    add_executable(${name} ${name}.c)
    target_link_libraries(${name} PRIVATE pcmon_test_support pcmon_warnings)
    add_test(NAME ${name} COMMAND ${name})
endforeach()

# Writes the fake machine to a folder; handy for running pcmon-sensors against it by hand.
add_executable(make_fixture make_fixture.c)
target_link_libraries(make_fixture PRIVATE pcmon_test_support pcmon_warnings)

add_test(NAME SourceCheck.Repo COMMAND bash ${CMAKE_SOURCE_DIR}/tools/check_sources.sh)
add_test(NAME Format.Check COMMAND bash ${CMAKE_SOURCE_DIR}/tools/check_format.sh)
set_tests_properties(Format.Check PROPERTIES SKIP_RETURN_CODE 77)

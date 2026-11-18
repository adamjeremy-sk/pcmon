// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Tests for CPU load parsing, the model name and the CPU temperature sensor.

#include <glib.h>

#include "core/cpu.h"
#include "fixture.h"

static void check_times(const char *text, gint64 total, gint64 idle) {
    PcCpuTimes t = {0, 0};
    g_assert_true(pc_parse_cpu_times(text, &t));
    g_assert_cmpint(t.total, ==, total);
    g_assert_cmpint(t.idle, ==, idle);
}

// Expected values come from running the Python original on the same lines.
static void test_parse_cpu_times(void) {
    check_times("cpu  100 0 50 800 50 0 0 0 0 0\n", 1000, 850);
    check_times("cpu  1 2 3 4\n", 10, 4);
    check_times("intr 5\ncpu  7 8 9 10 11 12 13 14 15 16\n", 84, 21);
    check_times("cpu  1 2 3 4 5 6 7 8 9\n", 36, 9); // only the first eight numbers count
    PcCpuTimes t;
    g_assert_false(pc_parse_cpu_times("cpu  1 2 3\n", &t));
    g_assert_false(pc_parse_cpu_times("cpu  1 x 3 4 5\n", &t));
    g_assert_false(pc_parse_cpu_times("cpu0 1 2 3 4\n", &t));
    g_assert_false(pc_parse_cpu_times("", &t));
    g_assert_false(pc_parse_cpu_times(NULL, &t));
}

static void test_cpu_load(void) {
    PcCpuTimes a = {100, 50};
    PcCpuTimes b = {200, 100};
    double load = -1;
    g_assert_true(pc_cpu_load(&a, &b, &load));
    g_assert_cmpfloat(load, ==, 50.0);
    g_assert_false(pc_cpu_load(&a, &a, &load)); // no time passed
    PcCpuTimes c = {200, 40};                   // idle went backwards: clamp to 100
    g_assert_true(pc_cpu_load(&a, &c, &load));
    g_assert_cmpfloat(load, ==, 100.0);
}

static void test_cpu_name_collapses_spaces(void) {
    char *root = pc_fixture_make();
    char *proc = g_build_filename(root, "proc", NULL);
    char *name = pc_cpu_name(proc);
    g_assert_cmpstr(name, ==, "12th Gen Intel(R) Core(TM) i5-12600K");
    g_free(name);
    char *missing = g_build_filename(root, "nothing", NULL);
    name = pc_cpu_name(missing);
    g_assert_cmpstr(name, ==, "");
    g_free(name);
    g_free(missing);
    g_free(proc);
    pc_fixture_remove(root);
}

static void test_cpu_temp_prefers_package(void) {
    char *root = pc_fixture_make();
    char *sys = g_build_filename(root, "sys", NULL);
    PcTemp *t = pc_cpu_temp(sys);
    g_assert_nonnull(t);
    g_assert_cmpstr(t->label, ==, "Package id 0");
    g_assert_cmpfloat(t->value, ==, 33.0);
    pc_temp_free(t);
    g_free(sys);
    pc_fixture_remove(root);
}

static void test_cpu_temp_falls_back_to_thermal_zone(void) {
    char *root = pc_fixture_make();
    pc_fixture_write(root, "sys/class/hwmon/hwmon1/name", "something_else\n");
    pc_fixture_write(root, "sys/class/thermal/thermal_zone0/type", "acpitz\n");
    pc_fixture_write(root, "sys/class/thermal/thermal_zone0/temp", "20000\n");
    pc_fixture_write(root, "sys/class/thermal/thermal_zone1/type", "x86_pkg_temp\n");
    pc_fixture_write(root, "sys/class/thermal/thermal_zone1/temp", "41000\n");
    char *sys = g_build_filename(root, "sys", NULL);
    PcTemp *t = pc_cpu_temp(sys);
    g_assert_nonnull(t);
    g_assert_cmpstr(t->label, ==, "CPU");
    g_assert_cmpfloat(t->value, ==, 41.0);
    g_assert_false(t->crit.set);
    pc_temp_free(t);
    g_free(sys);
    pc_fixture_remove(root);
}

static void test_cpu_temp_none_when_nothing(void) {
    char *root = g_dir_make_tmp("pcmon-empty-XXXXXX", NULL);
    PcTemp *t = pc_cpu_temp(root);
    g_assert_null(t);
    pc_fixture_remove(root);
}

int main(int argc, char **argv) {
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/cpu/parse_cpu_times", test_parse_cpu_times);
    g_test_add_func("/cpu/load", test_cpu_load);
    g_test_add_func("/cpu/name_collapses_spaces", test_cpu_name_collapses_spaces);
    g_test_add_func("/cpu/temp_prefers_package", test_cpu_temp_prefers_package);
    g_test_add_func("/cpu/temp_falls_back_to_thermal_zone",
                    test_cpu_temp_falls_back_to_thermal_zone);
    g_test_add_func("/cpu/temp_none_when_nothing", test_cpu_temp_none_when_nothing);
    return g_test_run();
}

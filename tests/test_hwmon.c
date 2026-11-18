// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Tests for reading the temperatures and fans of a hwmon directory.

#include <glib.h>

#include "core/hwmon.h"
#include "fixture.h"

static char *hwmon_dir(const char *root, const char *name) {
    return g_build_filename(root, "sys", "class", "hwmon", name, NULL);
}

static void test_temps_have_labels_and_limits(void) {
    char *root = pc_fixture_make();
    char *dir = hwmon_dir(root, "hwmon1");
    GPtrArray *temps = pc_hwmon_temps(dir);
    g_assert_cmpuint(temps->len, ==, 2);
    const PcTemp *package = g_ptr_array_index(temps, 0);
    g_assert_cmpstr(package->label, ==, "Package id 0");
    g_assert_cmpfloat(package->value, ==, 33.0);
    g_assert_true(package->crit.set);
    g_assert_cmpfloat(package->crit.value, ==, 100.0); // crit wins over max
    const PcTemp *core = g_ptr_array_index(temps, 1);
    g_assert_cmpstr(core->label, ==, "Core 0");
    g_assert_true(core->crit.set);
    g_assert_cmpfloat(core->crit.value, ==, 80.0); // crit is 0, so max is used
    g_ptr_array_unref(temps);
    g_free(dir);
    pc_fixture_remove(root);
}

static void test_temp_without_label_or_limit(void) {
    char *root = pc_fixture_make();
    char *dir = hwmon_dir(root, "hwmon3");
    GPtrArray *temps = pc_hwmon_temps(dir);
    g_assert_cmpuint(temps->len, ==, 1);
    const PcTemp *t = g_ptr_array_index(temps, 0);
    g_assert_cmpstr(t->label, ==, "temp1");
    g_assert_cmpfloat(t->value, ==, 38.5);
    g_assert_false(t->crit.set);
    g_ptr_array_unref(temps);
    g_free(dir);
    pc_fixture_remove(root);
}

static void test_fans_with_prefix_and_labels(void) {
    char *root = pc_fixture_make();
    char *dir = hwmon_dir(root, "hwmon3");
    GPtrArray *fans = pc_hwmon_fans(dir, "nct6687");
    g_assert_cmpuint(fans->len, ==, 4);
    const PcFan *first = g_ptr_array_index(fans, 0);
    g_assert_cmpstr(first->label, ==, "nct6687 fan1");
    g_assert_cmpfloat(first->value, ==, 858.0);
    g_assert_false(first->percent);
    const PcFan *labelled = g_ptr_array_index(fans, 3);
    g_assert_cmpstr(labelled->label, ==, "nct6687 Top case");
    g_ptr_array_unref(fans);
    fans = pc_hwmon_fans(dir, "");
    g_assert_cmpstr(((const PcFan *)g_ptr_array_index(fans, 1))->label, ==, "fan2");
    g_ptr_array_unref(fans);
    g_free(dir);
    pc_fixture_remove(root);
}

static void test_missing_directory_is_empty(void) {
    GPtrArray *temps = pc_hwmon_temps("/this/does/not/exist");
    GPtrArray *fans = pc_hwmon_fans("/this/does/not/exist", "x");
    g_assert_cmpuint(temps->len, ==, 0);
    g_assert_cmpuint(fans->len, ==, 0);
    g_ptr_array_unref(temps);
    g_ptr_array_unref(fans);
}

int main(int argc, char **argv) {
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/hwmon/temps_have_labels_and_limits", test_temps_have_labels_and_limits);
    g_test_add_func("/hwmon/temp_without_label_or_limit", test_temp_without_label_or_limit);
    g_test_add_func("/hwmon/fans_with_prefix_and_labels", test_fans_with_prefix_and_labels);
    g_test_add_func("/hwmon/missing_directory_is_empty", test_missing_directory_is_empty);
    return g_test_run();
}

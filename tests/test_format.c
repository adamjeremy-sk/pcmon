// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Tests for the display text of temperatures and fans.

#include <glib.h>

#include "core/format.h"

static void check_temp_name(const char *label, const char *expected) {
    char *text = pc_temp_name(label);
    g_assert_cmpstr(text, ==, expected);
    g_free(text);
}

static void test_temp_names(void) {
    check_temp_name("edge", "Edge temp");
    check_temp_name("Junction", "Junction temp");
    check_temp_name("MEM", "Memory temp");
    check_temp_name("temp3", "Temperature");
    check_temp_name("temp", "temp");
    check_temp_name("temp3x", "temp3x");
    check_temp_name("Package id 0", "Package id 0");
}

static void test_temp_frac(void) {
    PcTemp t = {(char *)"x", 50.0, pc_some(100.0)};
    g_assert_cmpfloat(pc_temp_frac(&t), ==, 0.5);
    t.crit = pc_none(); // no limit: 100 C
    g_assert_cmpfloat(pc_temp_frac(&t), ==, 0.5);
    t.crit = pc_some(0.0); // a zero limit counts as none, as in the Python original
    g_assert_cmpfloat(pc_temp_frac(&t), ==, 0.5);
    t.value = 150.0;
    g_assert_cmpfloat(pc_temp_frac(&t), ==, 1.0);
    t.value = -5.0;
    g_assert_cmpfloat(pc_temp_frac(&t), ==, 0.0);
}

static void check_fan_text(double value, gboolean percent, const char *expected) {
    PcFan f = {(char *)"fan1", value, percent};
    char *text = pc_format_fan(&f);
    g_assert_cmpstr(text, ==, expected);
    g_free(text);
}

static void test_format_fan(void) {
    check_fan_text(0, FALSE, "0 RPM");
    check_fan_text(858, FALSE, "858 RPM");
    check_fan_text(1127, FALSE, "1,127 RPM");
    check_fan_text(1234567, FALSE, "1,234,567 RPM");
    check_fan_text(857.6, FALSE, "858 RPM");
    check_fan_text(35, TRUE, "35%");
    check_fan_text(34.6, TRUE, "35%");
}

static void check_fan_name(const char *label, guint count, const char *expected) {
    PcFan f = {(char *)label, 0, FALSE};
    char *text = pc_fan_name(&f, count);
    g_assert_cmpstr(text, ==, expected);
    g_free(text);
}

static void test_fan_name(void) {
    check_fan_name("fan1", 1, "Fan");
    check_fan_name("fan1", 2, "Fan 1");
    check_fan_name("FAN12", 3, "Fan 12");
    check_fan_name("Fan2", 2, "Fan 2");
    check_fan_name("fan", 2, "fan");
    check_fan_name("fan1x", 2, "fan1x");
    check_fan_name("nct6687 fan1", 2, "nct6687 fan1");
}

static void test_fan_id(void) {
    PcFan f = {(char *)"nct6687 fan1", 0, FALSE};
    char *id = pc_fan_id(&f);
    g_assert_cmpstr(id, ==, "fan1");
    g_free(id);
    PcFan h = {(char *)"fan3", 0, FALSE};
    id = pc_fan_id(&h);
    g_assert_cmpstr(id, ==, "fan3");
    g_free(id);
}

int main(int argc, char **argv) {
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/format/temp_names", test_temp_names);
    g_test_add_func("/format/temp_frac", test_temp_frac);
    g_test_add_func("/format/format_fan", test_format_fan);
    g_test_add_func("/format/fan_name", test_fan_name);
    g_test_add_func("/format/fan_id", test_fan_id);
    return g_test_run();
}

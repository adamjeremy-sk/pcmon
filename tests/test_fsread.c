// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Tests for file reading, number parsing and natural ordering of sysfs names.

#include <glib.h>

#include "core/fsread.h"
#include "fixture.h"

static void test_tail_number(void) {
    g_assert_cmpint(pc_tail_number("hwmon12"), ==, 12);
    g_assert_cmpint(pc_tail_number("card1"), ==, 1);
    g_assert_cmpint(pc_tail_number("temp3_input"), ==, 3);
    g_assert_cmpint(pc_tail_number("card1-DP-1"), ==, 1);
    g_assert_cmpint(pc_tail_number("cpu"), ==, 0);
    g_assert_cmpint(pc_tail_number(""), ==, 0);
    g_assert_cmpint(pc_tail_number("/sys/class/hwmon/hwmon7/fan2_input"), ==, 2);
}

static void test_parse_int(void) {
    gint64 v = 0;
    g_assert_true(pc_parse_int("42", &v));
    g_assert_cmpint(v, ==, 42);
    g_assert_true(pc_parse_int("  -7\n", &v));
    g_assert_cmpint(v, ==, -7);
    g_assert_false(pc_parse_int("", &v));
    g_assert_false(pc_parse_int("4x", &v));
    g_assert_false(pc_parse_int("4.5", &v));
    g_assert_false(pc_parse_int(NULL, &v));
}

static void test_parse_double(void) {
    double v = 0;
    g_assert_true(pc_parse_double("54", &v));
    g_assert_cmpfloat(v, ==, 54.0);
    g_assert_true(pc_parse_double(" 35.5 ", &v));
    g_assert_cmpfloat(v, ==, 35.5);
    g_assert_false(pc_parse_double("[N/A]", &v));
    g_assert_false(pc_parse_double("", &v));
    g_assert_false(pc_parse_double("nan", &v));
    g_assert_false(pc_parse_double("inf", &v));
}

static void test_glob_orders_naturally(void) {
    char *root = pc_fixture_make();
    g_assert_nonnull(root);
    pc_fixture_write(root, "d/hwmon10/x", "1");
    pc_fixture_write(root, "d/hwmon2/x", "1");
    pc_fixture_write(root, "d/hwmon1/x", "1");
    pc_fixture_write(root, "d/other/x", "1");
    char *dir = g_build_filename(root, "d", NULL);
    GPtrArray *found = pc_glob_dir(dir, "hwmon*");
    g_assert_cmpuint(found->len, ==, 3);
    char *base = g_path_get_basename(g_ptr_array_index(found, 0));
    g_assert_cmpstr(base, ==, "hwmon1");
    g_free(base);
    base = g_path_get_basename(g_ptr_array_index(found, 2));
    g_assert_cmpstr(base, ==, "hwmon10");
    g_free(base);
    g_ptr_array_unref(found);
    found = pc_glob_dir("/this/does/not/exist", "*");
    g_assert_cmpuint(found->len, ==, 0);
    g_ptr_array_unref(found);
    g_free(dir);
    pc_fixture_remove(root);
}

static void test_read_text_and_int(void) {
    char *root = pc_fixture_make();
    pc_fixture_write(root, "a", "  hello \n");
    pc_fixture_write(root, "n", "621\n");
    char *path = g_build_filename(root, "a", NULL);
    char *text = pc_read_text(path);
    g_assert_cmpstr(text, ==, "hello");
    g_free(text);
    g_free(path);
    path = g_build_filename(root, "n", NULL);
    gint64 v = 0;
    g_assert_true(pc_read_int(path, &v));
    g_assert_cmpint(v, ==, 621);
    g_free(path);
    path = g_build_filename(root, "missing", NULL);
    g_assert_null(pc_read_text(path));
    g_assert_false(pc_read_int(path, &v));
    g_free(path);
    pc_fixture_remove(root);
}

int main(int argc, char **argv) {
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/fsread/tail_number", test_tail_number);
    g_test_add_func("/fsread/parse_int", test_parse_int);
    g_test_add_func("/fsread/parse_double", test_parse_double);
    g_test_add_func("/fsread/glob_orders_naturally", test_glob_orders_naturally);
    g_test_add_func("/fsread/read_text_and_int", test_read_text_and_int);
    return g_test_run();
}

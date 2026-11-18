// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Tests for the /proc/meminfo parser.

#include <glib.h>

#include "core/ram.h"

static void test_used_is_total_minus_available(void) {
    PcRam r = pc_parse_meminfo("MemTotal: 1000 kB\nMemFree: 100 kB\nMemAvailable: 400 kB\n");
    g_assert_true(r.present);
    g_assert_cmpfloat(r.total, ==, 1000.0 * 1024);
    g_assert_cmpfloat(r.used, ==, 600.0 * 1024);
}

static void test_falls_back_to_free(void) {
    PcRam r = pc_parse_meminfo("MemTotal: 1000 kB\nMemFree: 100 kB\n");
    g_assert_true(r.present);
    g_assert_cmpfloat(r.used, ==, 900.0 * 1024);
}

static void test_no_available_or_free_means_all_used(void) {
    PcRam r = pc_parse_meminfo("MemTotal: 1000 kB\n");
    g_assert_true(r.present);
    g_assert_cmpfloat(r.used, ==, 1000.0 * 1024);
}

static void test_missing_total_is_not_present(void) {
    g_assert_false(pc_parse_meminfo("MemFree: 100 kB\n").present);
    g_assert_false(pc_parse_meminfo("MemTotal: 0 kB\n").present);
    g_assert_false(pc_parse_meminfo("").present);
    g_assert_false(pc_parse_meminfo(NULL).present);
}

static void test_similar_keys_do_not_match(void) {
    // "SwapTotal" and "MemTotalX" must not be taken for MemTotal.
    PcRam r = pc_parse_meminfo("SwapTotal: 999 kB\nMemTotalX: 5 kB\nMemTotal: 1000 kB\n");
    g_assert_cmpfloat(r.total, ==, 1000.0 * 1024);
}

int main(int argc, char **argv) {
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/ram/used_is_total_minus_available", test_used_is_total_minus_available);
    g_test_add_func("/ram/falls_back_to_free", test_falls_back_to_free);
    g_test_add_func("/ram/no_available_or_free", test_no_available_or_free_means_all_used);
    g_test_add_func("/ram/missing_total", test_missing_total_is_not_present);
    g_test_add_func("/ram/similar_keys", test_similar_keys_do_not_match);
    return g_test_run();
}

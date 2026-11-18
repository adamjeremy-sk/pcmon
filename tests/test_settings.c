// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Tests for loading and saving settings.ini.

#include <glib.h>

#include "core/settings.h"
#include "fixture.h"

static char *path_in(const char *root, const char *relative) {
    return g_build_filename(root, relative, NULL);
}

static void test_defaults(void) {
    PcSettings *s = pc_settings_default();
    g_assert_false(s->always_on_top);
    g_assert_cmpint(s->opacity, ==, 100);
    g_assert_cmpint(s->theme, ==, PC_THEME_SYSTEM);
    g_assert_true(s->show_shared_memory);
    g_assert_cmpuint(g_hash_table_size(s->fan_names), ==, 4);
    g_assert_cmpstr(g_hash_table_lookup(s->fan_names, "fan1"), ==, "CPU Tower");
    g_assert_cmpstr(g_hash_table_lookup(s->fan_names, "fan7"), ==, "Top case");
    pc_settings_free(s);
}

static void test_missing_file_gives_defaults(void) {
    PcSettings *s = pc_settings_load("/this/does/not/exist.ini", NULL);
    g_assert_cmpint(s->opacity, ==, 100);
    g_assert_cmpuint(g_hash_table_size(s->fan_names), ==, 4);
    pc_settings_free(s);
}

static void test_save_then_load(void) {
    char *root = pc_fixture_make();
    char *path = path_in(root, "cfg/pcmon/settings.ini"); // the folders are created on save
    PcSettings *s = pc_settings_default();
    s->always_on_top = TRUE;
    s->opacity = 80;
    s->theme = PC_THEME_DARK;
    s->show_shared_memory = FALSE;
    pc_settings_set_fan_name(s, "fan2", "Pump");
    pc_settings_set_fan_name(s, "fan5", ""); // removes it
    pc_settings_save(s, path);
    pc_settings_free(s);

    PcSettings *back = pc_settings_load(path, NULL);
    g_assert_true(back->always_on_top);
    g_assert_cmpint(back->opacity, ==, 80);
    g_assert_cmpint(back->theme, ==, PC_THEME_DARK);
    g_assert_false(back->show_shared_memory);
    g_assert_cmpstr(g_hash_table_lookup(back->fan_names, "fan2"), ==, "Pump");
    g_assert_cmpstr(g_hash_table_lookup(back->fan_names, "fan1"), ==, "CPU Tower");
    g_assert_null(g_hash_table_lookup(back->fan_names, "fan5"));
    pc_settings_free(back);
    g_free(path);
    pc_fixture_remove(root);
}

static void test_theme_reads_any_case_and_ignores_junk(void) {
    char *root = pc_fixture_make();
    pc_fixture_write(root, "d.ini", "[window]\ntheme =  DARK \n");
    pc_fixture_write(root, "j.ini", "[window]\ntheme = purple\n");
    char *dark = path_in(root, "d.ini");
    char *junk = path_in(root, "j.ini");
    PcSettings *a = pc_settings_load(dark, NULL);
    PcSettings *b = pc_settings_load(junk, NULL);
    g_assert_cmpint(a->theme, ==, PC_THEME_DARK);
    g_assert_cmpint(b->theme, ==, PC_THEME_SYSTEM);
    pc_settings_free(a);
    pc_settings_free(b);
    g_free(dark);
    g_free(junk);
    pc_fixture_remove(root);
}

static void test_a_saved_empty_fan_list_replaces_the_defaults(void) {
    char *root = pc_fixture_make();
    char *path = path_in(root, "settings.ini");
    PcSettings *s = pc_settings_default();
    g_hash_table_remove_all(s->fan_names);
    pc_settings_save(s, path);
    pc_settings_free(s);
    PcSettings *back = pc_settings_load(path, NULL);
    g_assert_cmpuint(g_hash_table_size(back->fan_names), ==, 0);
    pc_settings_free(back);
    g_free(path);
    pc_fixture_remove(root);
}

static void test_by_hand_edits_are_read_leniently(void) {
    char *root = pc_fixture_make();
    pc_fixture_write(root, "a.ini",
                     "[window]\nalways_on_top = Yes\nopacity = 5\n[display]\n"
                     "show_shared_memory = off\n[fans]\nfan3 =   Side  \nfan4 =\n");
    char *path = path_in(root, "a.ini");
    PcSettings *s = pc_settings_load(path, NULL);
    g_assert_true(s->always_on_top);
    g_assert_cmpint(s->opacity, ==, PC_MIN_OPACITY); // clamped up
    g_assert_false(s->show_shared_memory);
    g_assert_cmpuint(g_hash_table_size(s->fan_names), ==, 1); // empty name = no name
    g_assert_cmpstr(g_hash_table_lookup(s->fan_names, "fan3"), ==, "Side");
    pc_settings_free(s);
    g_free(path);

    pc_fixture_write(root, "b.ini", "[window]\nopacity = lots\nalways_on_top = maybe\n");
    path = path_in(root, "b.ini");
    s = pc_settings_load(path, NULL);
    g_assert_cmpint(s->opacity, ==, 100);
    g_assert_false(s->always_on_top);
    g_assert_cmpuint(g_hash_table_size(s->fan_names), ==, 4); // no [fans] group: defaults stay
    pc_settings_free(s);
    g_free(path);

    pc_fixture_write(root, "c.ini", "[window]\nopacity = 250\n");
    path = path_in(root, "c.ini");
    s = pc_settings_load(path, NULL);
    g_assert_cmpint(s->opacity, ==, 100); // clamped down
    pc_settings_free(s);
    g_free(path);
    pc_fixture_remove(root);
}

static void test_unreadable_file_falls_back_to_the_older_one(void) {
    char *root = pc_fixture_make();
    pc_fixture_write(root, "bad.ini", "this is not an ini file\n");
    pc_fixture_write(root, "old.ini", "[window]\nopacity = 60\n[fans]\nfan9 = Old name\n");
    char *bad = path_in(root, "bad.ini");
    char *old = path_in(root, "old.ini");
    PcSettings *s = pc_settings_load(bad, old);
    g_assert_cmpint(s->opacity, ==, 60);
    g_assert_cmpstr(g_hash_table_lookup(s->fan_names, "fan9"), ==, "Old name");
    pc_settings_free(s);
    s = pc_settings_load(bad, NULL);
    g_assert_cmpint(s->opacity, ==, 100);
    pc_settings_free(s);
    g_free(bad);
    g_free(old);
    pc_fixture_remove(root);
}

static void test_default_paths(void) {
    char *p = pc_settings_default_path();
    char *l = pc_settings_legacy_path();
    g_assert_true(g_str_has_suffix(p, "/pcmon/settings.ini"));
    g_assert_true(g_str_has_suffix(l, "/pymon/settings.ini"));
    g_free(p);
    g_free(l);
}

int main(int argc, char **argv) {
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/settings/defaults", test_defaults);
    g_test_add_func("/settings/missing_file_gives_defaults", test_missing_file_gives_defaults);
    g_test_add_func("/settings/save_then_load", test_save_then_load);
    g_test_add_func("/settings/theme", test_theme_reads_any_case_and_ignores_junk);
    g_test_add_func("/settings/empty_fan_list", test_a_saved_empty_fan_list_replaces_the_defaults);
    g_test_add_func("/settings/by_hand_edits", test_by_hand_edits_are_read_leniently);
    g_test_add_func("/settings/legacy_fallback", test_unreadable_file_falls_back_to_the_older_one);
    g_test_add_func("/settings/default_paths", test_default_paths);
    return g_test_run();
}

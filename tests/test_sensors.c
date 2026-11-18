// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Tests for the whole snapshot (CPU, GPUs, system fans, RAM) and its JSON text.

#include <string.h>

#include <glib.h>

#include "core/json.h"
#include "core/sensors.h"
#include "fixture.h"

static void test_snapshot_of_the_fixture_machine(void) {
    char *root = pc_fixture_make();
    PcSensors *sensors = pc_sensors_new(root);
    PcSnapshot *snap = pc_sensors_collect(sensors);

    g_assert_cmpstr(snap->cpu.name, ==, "12th Gen Intel(R) Core(TM) i5-12600K");
    g_assert_true(snap->cpu.has_temp);
    g_assert_cmpstr(snap->cpu.temp.label, ==, "Package id 0");
    g_assert_false(snap->cpu.load.set); // /proc/stat does not change in a fake tree

    g_assert_cmpuint(snap->gpus->len, ==, 2);

    // The AMD GPU's fan belongs to the GPU, so it is not a system fan. The chip without a
    // name file is listed under the folder's name. The stopped fan is still reported.
    g_assert_cmpuint(snap->fans->len, ==, 5);
    g_assert_cmpstr(((const PcFan *)g_ptr_array_index(snap->fans, 0))->label, ==, "nct6687 fan1");
    g_assert_cmpfloat(((const PcFan *)g_ptr_array_index(snap->fans, 1))->value, ==, 0.0);
    g_assert_cmpstr(((const PcFan *)g_ptr_array_index(snap->fans, 4))->label, ==, "hwmon11 fan1");

    g_assert_true(snap->ram.present);
    g_assert_cmpfloat(snap->ram.total, ==, 32575916.0 * 1024);
    g_assert_cmpfloat(snap->ram.used, ==, (32575916.0 - 20000000.0) * 1024);

    pc_snapshot_free(snap);
    pc_sensors_free(sensors);
    pc_fixture_remove(root);
}

static void test_cpu_load_between_two_reads(void) {
    char *root = pc_fixture_make();
    PcSensors *sensors = pc_sensors_new(root);
    PcSnapshot *first = pc_sensors_collect(sensors);
    pc_snapshot_free(first);
    // 1000 -> 2000 jiffies in total, 850 -> 1350 of them idle: 50% busy.
    pc_fixture_write(root, "proc/stat", "cpu  400 0 250 1300 50 0 0 0 0 0\n");
    PcSnapshot *second = pc_sensors_collect(sensors);
    g_assert_true(second->cpu.load.set);
    g_assert_cmpfloat_with_epsilon(second->cpu.load.value, 50.0, 1e-9);
    pc_snapshot_free(second);
    pc_sensors_free(sensors);
    pc_fixture_remove(root);
}

static void test_an_empty_machine_gives_an_empty_snapshot(void) {
    char *root = g_dir_make_tmp("pcmon-empty-XXXXXX", NULL);
    PcSensors *sensors = pc_sensors_new(root);
    PcSnapshot *snap = pc_sensors_collect(sensors);
    g_assert_cmpstr(snap->cpu.name, ==, "");
    g_assert_false(snap->cpu.has_temp);
    g_assert_false(snap->cpu.load.set);
    g_assert_cmpuint(snap->gpus->len, ==, 0);
    g_assert_cmpuint(snap->fans->len, ==, 0);
    g_assert_false(snap->ram.present);
    char *json = pc_snapshot_to_json(snap);
    g_assert_nonnull(strstr(json, "\"temp\": null"));
    g_assert_nonnull(strstr(json, "\"gpus\": []"));
    g_assert_nonnull(strstr(json, "\"ram\": null"));
    g_free(json);
    pc_snapshot_free(snap);
    pc_sensors_free(sensors);
    pc_fixture_remove(root);
}

static void test_json_numbers_and_strings(void) {
    char *root = pc_fixture_make();
    pc_fixture_write(root, "proc/cpuinfo", "model name\t: A \"quoted\" \\ CPU\n");
    pc_fixture_write(root, "sys/class/hwmon/hwmon1/temp1_input", "33500\n");
    PcSensors *sensors = pc_sensors_new(root);
    PcSnapshot *snap = pc_sensors_collect(sensors);
    char *json = pc_snapshot_to_json(snap);
    g_assert_nonnull(strstr(json, "\"name\": \"A \\\"quoted\\\" \\\\ CPU\""));
    g_assert_nonnull(strstr(json, "\"value\": 33.5"));
    g_assert_nonnull(strstr(json, "\"vram_total\": 27632381952"));
    g_assert_nonnull(strstr(json, "\"shared_memory\": true"));
    g_free(json);
    pc_snapshot_free(snap);
    pc_sensors_free(sensors);
    pc_fixture_remove(root);
}

int main(int argc, char **argv) {
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/sensors/snapshot_of_the_fixture_machine",
                    test_snapshot_of_the_fixture_machine);
    g_test_add_func("/sensors/cpu_load_between_two_reads", test_cpu_load_between_two_reads);
    g_test_add_func("/sensors/empty_machine", test_an_empty_machine_gives_an_empty_snapshot);
    g_test_add_func("/sensors/json_numbers_and_strings", test_json_numbers_and_strings);
    return g_test_run();
}

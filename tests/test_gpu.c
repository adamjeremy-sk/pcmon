// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Tests for GPU discovery, lspci name cleaning and nvidia-smi parsing.

#include <glib.h>

#include "core/gpu.h"
#include "core/nvidia.h"
#include "fixture.h"

static void check_name(const char *line, const char *vendor, const char *expected) {
    char *name = pc_pretty_gpu_name(line, vendor);
    g_assert_cmpstr(name, ==, expected);
    g_free(name);
}

// Expected values come from running the Python original on the same lines.
static void test_pretty_gpu_name(void) {
    check_name("03:00.0 \"VGA compatible controller\" \"Advanced Micro Devices, Inc. [AMD/ATI]\" "
               "\"Navi 31 [Radeon RX 7900 XTX]\" -rc8 -p00 \"XFX\" \"Speedster\"",
               "AMD", "AMD Radeon RX 7900 XTX");
    check_name("00:02.0 \"VGA compatible controller\" \"Intel Corporation\" "
               "\"AlderLake-S GT1 [UHD Graphics 770]\" -r0c",
               "Intel", "Intel UHD Graphics 770");
    check_name("01:00.0 \"VGA\" \"NVIDIA Corporation\" \"GA104 [GeForce RTX 3070]\" -ra1", "NVIDIA",
               "NVIDIA GeForce RTX 3070");
    check_name("01:00.0 \"VGA\" \"Vendor\" \"Plain Name\" -ra1", "GPU", "Plain Name");
    check_name("01:00.0 \"VGA\" \"Vendor\"", "AMD", NULL);
    check_name("", "AMD", NULL);
    check_name("a \"unterminated", "AMD", NULL);
    check_name(NULL, "AMD", NULL);
}

static void test_parse_nvidia_smi(void) {
    GHashTable *t =
        pc_parse_nvidia_smi("00000000:01:00.0, NVIDIA GeForce RTX 3070, 54, 1024, 8192, 35, 12\n"
                            "00000000:02:00.0, Tesla T4, [N/A], 0, 15360, [N/A]\n");
    g_assert_cmpuint(g_hash_table_size(t), ==, 2);
    const PcNvidiaGpu *a = g_hash_table_lookup(t, "0000:01:00.0");
    g_assert_nonnull(a);
    g_assert_cmpstr(a->name, ==, "NVIDIA GeForce RTX 3070");
    g_assert_true(a->temp.set);
    g_assert_cmpfloat(a->temp.value, ==, 54.0);
    g_assert_cmpfloat(a->used.value, ==, 1073741824.0);
    g_assert_cmpfloat(a->total.value, ==, 8589934592.0);
    g_assert_cmpfloat(a->fan_pct.value, ==, 35.0);
    g_assert_cmpfloat(a->load.value, ==, 12.0);
    const PcNvidiaGpu *b = g_hash_table_lookup(t, "0000:02:00.0");
    g_assert_nonnull(b);
    g_assert_false(b->temp.set);
    g_assert_false(b->fan_pct.set);
    g_assert_false(b->load.set); // no seventh column
    g_assert_cmpfloat(b->total.value, ==, 16106127360.0);
    g_hash_table_unref(t);

    t = pc_parse_nvidia_smi("garbage\n");
    g_assert_cmpuint(g_hash_table_size(t), ==, 0);
    g_hash_table_unref(t);
    t = pc_parse_nvidia_smi(NULL);
    g_assert_cmpuint(g_hash_table_size(t), ==, 0);
    g_hash_table_unref(t);
}

static void test_reader_finds_both_gpus(void) {
    char *root = pc_fixture_make();
    char *sys = g_build_filename(root, "sys", NULL);
    PcGpuReader *reader = pc_gpu_reader_new(sys, FALSE);
    GHashTable *devs = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    GPtrArray *gpus = pc_gpu_reader_read(reader, devs);
    // card2-DP-1 is a connector and is skipped; card1 comes before card2.
    g_assert_cmpuint(gpus->len, ==, 2);
    g_assert_cmpuint(g_hash_table_size(devs), ==, 2);

    const PcGpu *intel = g_ptr_array_index(gpus, 0);
    g_assert_cmpstr(intel->name, ==, "Intel GPU (card1)");
    g_assert_cmpstr(intel->vendor, ==, "Intel");
    g_assert_cmpstr(intel->slot, ==, "0000:00:02.0");
    g_assert_true(intel->shared_memory);
    g_assert_false(intel->load.set);
    g_assert_cmpuint(intel->temps->len, ==, 0);

    const PcGpu *amd = g_ptr_array_index(gpus, 1);
    g_assert_cmpstr(amd->name, ==, "AMD Radeon RX 7900 XTX");
    g_assert_cmpstr(amd->slot, ==, "0000:03:00.0");
    g_assert_false(amd->shared_memory);
    g_assert_cmpfloat(amd->load.value, ==, 6.0);
    g_assert_cmpfloat(amd->vram_total.value, ==, 27632381952.0);
    g_assert_cmpuint(amd->temps->len, ==, 3);
    g_assert_cmpstr(((const PcTemp *)g_ptr_array_index(amd->temps, 1))->label, ==, "junction");
    g_assert_cmpuint(amd->fans->len, ==, 1);
    g_assert_cmpfloat(((const PcFan *)g_ptr_array_index(amd->fans, 0))->value, ==, 621.0);

    g_ptr_array_unref(gpus);
    g_hash_table_unref(devs);
    pc_gpu_reader_free(reader);
    g_free(sys);
    pc_fixture_remove(root);
}

static void test_card_without_vendor_is_skipped(void) {
    char *root = pc_fixture_make();
    pc_fixture_write(root, "sys/class/drm/card7/device/uevent", "PCI_SLOT_NAME=0000:09:00.0\n");
    char *sys = g_build_filename(root, "sys", NULL);
    PcGpuReader *reader = pc_gpu_reader_new(sys, FALSE);
    GPtrArray *gpus = pc_gpu_reader_read(reader, NULL);
    g_assert_cmpuint(gpus->len, ==, 2);
    g_ptr_array_unref(gpus);
    pc_gpu_reader_free(reader);
    g_free(sys);
    pc_fixture_remove(root);
}

static void test_unknown_vendor_and_name_fallback(void) {
    char *root = pc_fixture_make();
    pc_fixture_write(root, "sys/class/drm/card5/device/vendor", "0x1234\n");
    char *sys = g_build_filename(root, "sys", NULL);
    PcGpuReader *reader = pc_gpu_reader_new(sys, FALSE);
    GPtrArray *gpus = pc_gpu_reader_read(reader, NULL);
    g_assert_cmpuint(gpus->len, ==, 3);
    const PcGpu *odd = g_ptr_array_index(gpus, 2);
    g_assert_cmpstr(odd->vendor, ==, "GPU");
    g_assert_cmpstr(odd->name, ==, "GPU GPU (card5)");
    g_assert_null(odd->slot);
    g_assert_false(odd->shared_memory);
    g_ptr_array_unref(gpus);
    pc_gpu_reader_free(reader);
    g_free(sys);
    pc_fixture_remove(root);
}

int main(int argc, char **argv) {
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/gpu/pretty_gpu_name", test_pretty_gpu_name);
    g_test_add_func("/gpu/parse_nvidia_smi", test_parse_nvidia_smi);
    g_test_add_func("/gpu/reader_finds_both_gpus", test_reader_finds_both_gpus);
    g_test_add_func("/gpu/card_without_vendor_is_skipped", test_card_without_vendor_is_skipped);
    g_test_add_func("/gpu/unknown_vendor_and_name_fallback", test_unknown_vendor_and_name_fallback);
    return g_test_run();
}

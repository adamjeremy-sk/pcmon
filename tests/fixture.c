// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// The table of files that make up the fake machine, and the code that writes them.

#include "fixture.h"

#include <glib/gstdio.h>
#include <unistd.h>

typedef struct {
    const char *path;
    const char *content; // file text, or the link target when is_link
    gboolean is_link;
} Entry;

#define FILE_(path, content)                                                                       \
    { path, content, FALSE }
#define LINK_(path, target)                                                                        \
    { path, target, TRUE }

#define AMD_DEV "sys/devices/pci0000:00/0000:03:00.0"
#define INTEL_DEV "sys/devices/pci0000:00/0000:00:02.0"

// A desktop with an Intel CPU (coretemp), an AMD discrete GPU, an Intel iGPU, a Nuvoton
// motherboard chip, an NVMe drive and a stopped fan: every case the readers have to handle.
static const Entry kEntries[] = {
    // acpitz placeholder, then the CPU
    FILE_("sys/class/hwmon/hwmon0/name", "acpitz\n"),
    FILE_("sys/class/hwmon/hwmon0/temp1_input", "27800\n"),
    FILE_("sys/class/hwmon/hwmon1/name", "coretemp\n"),
    FILE_("sys/class/hwmon/hwmon1/temp1_input", "33000\n"),
    FILE_("sys/class/hwmon/hwmon1/temp1_label", "Package id 0\n"),
    FILE_("sys/class/hwmon/hwmon1/temp1_max", "80000\n"),
    FILE_("sys/class/hwmon/hwmon1/temp1_crit", "100000\n"),
    FILE_("sys/class/hwmon/hwmon1/temp2_input", "30000\n"),
    FILE_("sys/class/hwmon/hwmon1/temp2_label", "Core 0\n"),
    FILE_("sys/class/hwmon/hwmon1/temp2_max", "80000\n"),
    FILE_("sys/class/hwmon/hwmon1/temp2_crit", "0\n"),
    // the AMD GPU: hwmon2 is a link into the PCI device, as on a real system
    LINK_("sys/class/hwmon/hwmon2", "../../devices/pci0000:00/0000:03:00.0/hwmon/hwmon2"),
    LINK_(AMD_DEV "/hwmon/hwmon2/device", "../.."),
    FILE_(AMD_DEV "/hwmon/hwmon2/name", "amdgpu\n"),
    FILE_(AMD_DEV "/hwmon/hwmon2/temp1_input", "45000\n"),
    FILE_(AMD_DEV "/hwmon/hwmon2/temp1_label", "edge\n"),
    FILE_(AMD_DEV "/hwmon/hwmon2/temp1_crit", "100000\n"),
    FILE_(AMD_DEV "/hwmon/hwmon2/temp2_input", "51000\n"),
    FILE_(AMD_DEV "/hwmon/hwmon2/temp2_label", "junction\n"),
    FILE_(AMD_DEV "/hwmon/hwmon2/temp2_crit", "110000\n"),
    FILE_(AMD_DEV "/hwmon/hwmon2/temp3_input", "65000\n"),
    FILE_(AMD_DEV "/hwmon/hwmon2/temp3_label", "mem\n"),
    FILE_(AMD_DEV "/hwmon/hwmon2/temp3_crit", "108000\n"),
    FILE_(AMD_DEV "/hwmon/hwmon2/fan1_input", "621\n"),
    FILE_(AMD_DEV "/vendor", "0x1002\n"),
    FILE_(AMD_DEV "/uevent", "DRIVER=amdgpu\nPCI_SLOT_NAME=0000:03:00.0\nPCI_ID=1002:744C\n"),
    FILE_(AMD_DEV "/product_name", "AMD Radeon RX 7900 XTX\n"),
    FILE_(AMD_DEV "/mem_info_vram_used", "25619263488\n"),
    FILE_(AMD_DEV "/mem_info_vram_total", "27632381952\n"),
    FILE_(AMD_DEV "/gpu_busy_percent", "6\n"),
    LINK_(AMD_DEV "/drm/card2/device", "../.."),
    LINK_("sys/class/drm/card2", "../../devices/pci0000:00/0000:03:00.0/drm/card2"),
    FILE_("sys/class/drm/card2-DP-1/status", "disconnected\n"),
    // the Intel integrated GPU: no hwmon, no VRAM, no product name
    FILE_(INTEL_DEV "/vendor", "0x8086\n"),
    FILE_(INTEL_DEV "/uevent", "DRIVER=i915\nPCI_SLOT_NAME=0000:00:02.0\n"),
    LINK_(INTEL_DEV "/drm/card1/device", "../.."),
    LINK_("sys/class/drm/card1", "../../devices/pci0000:00/0000:00:02.0/drm/card1"),
    // the motherboard chip: fan headers 1, 2, 5 and 7, one of them stopped
    FILE_("sys/class/hwmon/hwmon3/name", "nct6687\n"),
    FILE_("sys/class/hwmon/hwmon3/fan1_input", "858\n"),
    FILE_("sys/class/hwmon/hwmon3/fan2_input", "0\n"),
    FILE_("sys/class/hwmon/hwmon3/fan5_input", "1127\n"),
    FILE_("sys/class/hwmon/hwmon3/fan7_input", "1036\n"),
    FILE_("sys/class/hwmon/hwmon3/fan7_label", "Top case\n"),
    FILE_("sys/class/hwmon/hwmon3/temp1_input", "38500\n"),
    // a drive (no fans) and a chip without a name file that has a fan
    FILE_("sys/class/hwmon/hwmon10/name", "nvme\n"),
    FILE_("sys/class/hwmon/hwmon10/temp1_input", "40850\n"),
    FILE_("sys/class/hwmon/hwmon10/temp1_label", "Composite\n"),
    FILE_("sys/class/hwmon/hwmon11/fan1_input", "300\n"),
    // proc
    FILE_("proc/stat", "cpu  100 0 50 800 50 0 0 0 0 0\ncpu0 10 0 5 80 5 0 0 0 0 0\n"),
    FILE_("proc/cpuinfo", "processor\t: 0\nmodel name\t: 12th Gen  Intel(R) Core(TM) i5-12600K\n"),
    FILE_("proc/meminfo", "MemTotal:       32575916 kB\nMemFree:         1000000 kB\n"
                          "MemAvailable:   20000000 kB\nSwapTotal:       8388604 kB\n"),
};

gboolean pc_fixture_write(const char *root, const char *relative, const char *content) {
    char *path = g_build_filename(root, relative, NULL);
    char *dir = g_path_get_dirname(path);
    g_mkdir_with_parents(dir, 0755);
    gboolean ok = g_file_set_contents(path, content, -1, NULL);
    g_free(dir);
    g_free(path);
    return ok;
}

gboolean pc_fixture_link(const char *root, const char *relative, const char *target) {
    char *path = g_build_filename(root, relative, NULL);
    char *dir = g_path_get_dirname(path);
    g_mkdir_with_parents(dir, 0755);
    g_unlink(path);
    gboolean ok = symlink(target, path) == 0;
    g_free(dir);
    g_free(path);
    return ok;
}

gboolean pc_fixture_build(const char *root) {
    // The links to directories come after the directories they point into exist, so write the
    // plain files first and the links second.
    for (int pass = 0; pass < 2; pass++) {
        for (gsize i = 0; i < G_N_ELEMENTS(kEntries); i++) {
            const Entry *e = &kEntries[i];
            if (e->is_link != (pass == 1)) {
                continue;
            }
            gboolean ok = e->is_link ? pc_fixture_link(root, e->path, e->content)
                                     : pc_fixture_write(root, e->path, e->content);
            if (!ok) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

char *pc_fixture_make(void) {
    char *root = g_dir_make_tmp("pcmon-fixture-XXXXXX", NULL);
    if (root != NULL && !pc_fixture_build(root)) {
        g_free(root);
        return NULL;
    }
    return root;
}

void pc_fixture_remove(char *root) {
    if (root == NULL) {
        return;
    }
    char *argv[] = {"rm", "-rf", root, NULL};
    g_spawn_sync(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL, NULL, NULL);
    g_free(root);
}

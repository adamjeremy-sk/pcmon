// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// GPU discovery and readings from drm sysfs, hwmon, lspci names and nvidia-smi.

#include "core/gpu.h"

#include <string.h>

#include "core/fsread.h"
#include "core/hwmon.h"
#include "core/nvidia.h"
#include "core/proc_run.h"

struct PcGpuReader {
    char *sys;
    gboolean run_helpers;
    GHashTable *names; // slot or card path -> cached name
    PcNvidiaCache *smi;
};

char *pc_pretty_gpu_name(const char *lspci_line, const char *vendor_name) {
    if (lspci_line == NULL) {
        return NULL;
    }
    gint argc = 0;
    char **argv = NULL;
    if (!g_shell_parse_argv(lspci_line, &argc, &argv, NULL)) {
        return NULL;
    }
    if (argc < 4) {
        g_strfreev(argv);
        return NULL;
    }
    char *name = g_strdup(argv[3]);
    g_strfreev(argv);
    // "Navi 31 [Radeon RX 7900 XTX]" -> "Radeon RX 7900 XTX"
    const char *open = strchr(name, '[');
    const char *close = strrchr(name, ']');
    if (open != NULL && close != NULL && close > open) {
        char *inner = g_strndup(open + 1, (gsize)(close - open - 1));
        g_free(name);
        name = inner;
    }
    if (vendor_name != NULL && vendor_name[0] != '\0' && strcmp(vendor_name, "GPU") != 0) {
        char *low_name = g_ascii_strdown(name, -1);
        char *low_vendor = g_ascii_strdown(vendor_name, -1);
        if (strstr(low_name, low_vendor) == NULL) {
            char *with_vendor = g_strdup_printf("%s %s", vendor_name, name);
            g_free(name);
            name = with_vendor;
        }
        g_free(low_name);
        g_free(low_vendor);
    }
    return name;
}

PcGpuReader *pc_gpu_reader_new(const char *sys_root, gboolean run_helpers) {
    PcGpuReader *r = g_new0(PcGpuReader, 1);
    r->sys = g_strdup(sys_root);
    r->run_helpers = run_helpers;
    r->names = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    r->smi = pc_nvidia_cache_new(run_helpers);
    return r;
}

void pc_gpu_reader_free(PcGpuReader *reader) {
    if (reader == NULL) {
        return;
    }
    g_free(reader->sys);
    g_hash_table_unref(reader->names);
    pc_nvidia_cache_free(reader->smi);
    g_free(reader);
}

static const char *vendor_name(const char *vendor_id) {
    if (strcmp(vendor_id, "0x1002") == 0) {
        return "AMD";
    }
    if (strcmp(vendor_id, "0x8086") == 0) {
        return "Intel";
    }
    if (strcmp(vendor_id, "0x10de") == 0) {
        return "NVIDIA";
    }
    return "GPU";
}

// "card0", "card12": a card itself, not a connector such as "card1-DP-1".
static gboolean is_card_name(const char *name) {
    if (!g_str_has_prefix(name, "card") || name[4] == '\0') {
        return FALSE;
    }
    for (const char *p = name + 4; *p != '\0'; p++) {
        if (!g_ascii_isdigit(*p)) {
            return FALSE;
        }
    }
    return TRUE;
}

// PCI_SLOT_NAME from the device's uevent file, lower case; NULL when there is none.
static char *read_slot(const char *dev) {
    char *path = g_build_filename(dev, "uevent", NULL);
    char *text = pc_read_text(path);
    g_free(path);
    char *slot = NULL;
    if (text != NULL) {
        char **lines = g_strsplit(text, "\n", -1);
        for (char **line = lines; *line != NULL && slot == NULL; line++) {
            if (g_str_has_prefix(*line, "PCI_SLOT_NAME=")) {
                slot = g_ascii_strdown(*line + strlen("PCI_SLOT_NAME="), -1);
            }
        }
        g_strfreev(lines);
        g_free(text);
    }
    return slot;
}

static char *query_lspci_name(const char *slot, const char *vendor) {
    char *lspci = g_find_program_in_path("lspci");
    if (lspci == NULL) {
        return NULL;
    }
    char *argv[] = {lspci, "-mm", "-s", (char *)slot, NULL};
    char *out = pc_run_capture(argv, 2000);
    g_free(lspci);
    char *line = out != NULL ? g_strstrip(out) : NULL;
    char *name = pc_pretty_gpu_name(line, vendor);
    g_free(out);
    return name;
}

// The name of a GPU: product_name from the driver, else lspci, else "<vendor> GPU (cardN)".
static char *gpu_name(PcGpuReader *r, const char *dev, const char *card, const char *vendor,
                      const char *slot) {
    char *key = g_strdup(slot != NULL ? slot : card);
    const char *cached = g_hash_table_lookup(r->names, key);
    if (cached != NULL) {
        g_free(key);
        return g_strdup(cached);
    }
    char *product_path = g_build_filename(dev, "product_name", NULL);
    char *name = pc_read_text(product_path);
    g_free(product_path);
    if (name == NULL || name[0] == '\0') {
        g_free(name);
        name = (slot != NULL && r->run_helpers) ? query_lspci_name(slot, vendor) : NULL;
    }
    if (name == NULL || name[0] == '\0') {
        g_free(name);
        char *base = g_path_get_basename(card);
        name = g_strdup_printf("%s GPU (%s)", vendor, base);
        g_free(base);
    }
    g_hash_table_replace(r->names, key, g_strdup(name));
    return name;
}

static PcOpt read_opt_int(const char *dir, const char *file) {
    char *path = g_build_filename(dir, file, NULL);
    gint64 v;
    PcOpt o = pc_read_int(path, &v) ? pc_some((double)v) : pc_none();
    g_free(path);
    return o;
}

// Replaces what sysfs gave with what nvidia-smi reports for the same PCI slot.
static void apply_nvidia(PcGpu *gpu, const PcNvidiaGpu *s) {
    if (s->name != NULL && s->name[0] != '\0') {
        g_free(gpu->name);
        gpu->name = g_strdup(s->name);
    }
    if (s->temp.set) {
        g_ptr_array_set_size(gpu->temps, 0);
        g_ptr_array_add(gpu->temps, pc_temp_new("GPU", s->temp.value, pc_none()));
    }
    gpu->vram_used = s->used;
    gpu->vram_total = s->total;
    if (s->load.set) {
        gpu->load = s->load;
    }
    if (s->fan_pct.set) {
        g_ptr_array_set_size(gpu->fans, 0);
        g_ptr_array_add(gpu->fans, pc_fan_new("fan1", s->fan_pct.value, TRUE));
    }
}

static PcGpu *read_card(PcGpuReader *r, const char *card, const GHashTable *smi, char **dev_out) {
    char *dev = g_build_filename(card, "device", NULL);
    char *vendor_path = g_build_filename(dev, "vendor", NULL);
    char *vendor_text = pc_read_text(vendor_path);
    g_free(vendor_path);
    if (vendor_text == NULL || vendor_text[0] == '\0') {
        g_free(vendor_text);
        g_free(dev);
        return NULL;
    }
    char *vendor_id = g_ascii_strdown(vendor_text, -1);
    g_free(vendor_text);
    const char *vendor = vendor_name(vendor_id);
    g_free(vendor_id);

    PcGpu *gpu = g_new0(PcGpu, 1);
    gpu->slot = read_slot(dev);
    gpu->vendor = g_strdup(vendor);
    gpu->name = gpu_name(r, dev, card, vendor, gpu->slot);
    gpu->temps = pc_temp_array_new();
    gpu->fans = pc_fan_array_new();
    gpu->vram_used = read_opt_int(dev, "mem_info_vram_used");
    gpu->vram_total = read_opt_int(dev, "mem_info_vram_total");
    // AMD exposes this in sysfs. Intel has no unprivileged equivalent.
    gpu->load = read_opt_int(dev, "gpu_busy_percent");

    char *hwmon_parent = g_build_filename(dev, "hwmon", NULL);
    GPtrArray *hw = pc_glob_dir(hwmon_parent, "hwmon*");
    g_free(hwmon_parent);
    if (hw->len > 0) {
        g_ptr_array_unref(gpu->temps);
        g_ptr_array_unref(gpu->fans);
        gpu->temps = pc_hwmon_temps(g_ptr_array_index(hw, 0));
        gpu->fans = pc_hwmon_fans(g_ptr_array_index(hw, 0), "");
    }
    g_ptr_array_unref(hw);

    const PcNvidiaGpu *s =
        gpu->slot != NULL ? g_hash_table_lookup((GHashTable *)smi, gpu->slot) : NULL;
    if (s != NULL) {
        apply_nvidia(gpu, s); // the proprietary driver exposes nothing through sysfs
    }
    // Integrated Intel graphics use system RAM and have no dedicated VRAM.
    gpu->shared_memory =
        strcmp(vendor, "Intel") == 0 && (!gpu->vram_total.set || gpu->vram_total.value == 0);
    *dev_out = dev;
    return gpu;
}

GPtrArray *pc_gpu_reader_read(PcGpuReader *reader, GHashTable *devs) {
    GPtrArray *gpus = g_ptr_array_new_with_free_func((GDestroyNotify)pc_gpu_free);
    const GHashTable *smi = pc_nvidia_cache_get(reader->smi);
    char *drm = g_build_filename(reader->sys, "class", "drm", NULL);
    GPtrArray *cards = pc_glob_dir(drm, "card*");
    g_free(drm);
    for (guint i = 0; i < cards->len; i++) {
        const char *card = g_ptr_array_index(cards, i);
        char *base = g_path_get_basename(card);
        gboolean is_card = is_card_name(base);
        g_free(base);
        if (!is_card) {
            continue;
        }
        char *dev = NULL;
        PcGpu *gpu = read_card(reader, card, smi, &dev);
        if (gpu == NULL) {
            continue;
        }
        g_ptr_array_add(gpus, gpu);
        if (devs != NULL) {
            g_hash_table_add(devs, pc_realpath(dev));
        }
        g_free(dev);
    }
    g_ptr_array_unref(cards);
    return gpus;
}

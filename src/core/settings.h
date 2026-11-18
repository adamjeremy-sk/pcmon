// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// The saved options (settings.ini): window behaviour, display switches and fan names.

#pragma once

#include <glib.h>

#define PC_MIN_OPACITY 30 // percent; lower than this and the window is effectively invisible

typedef enum {
    PC_THEME_SYSTEM, // leave the desktop theme alone
    PC_THEME_LIGHT,
    PC_THEME_DARK,
} PcTheme;

typedef struct {
    gboolean always_on_top;
    int opacity; // PC_MIN_OPACITY..100
    gboolean show_shared_memory;
    PcTheme theme;
    GHashTable *fan_names; // fan id ("fan1") -> name; both g_free'd strings
} PcSettings;

// Defaults: not on top, opaque, shared-memory line shown, and the fan names of the author's case.
PcSettings *pc_settings_default(void);
void pc_settings_free(PcSettings *settings);

// Loads path; when it does not exist or cannot be read, tries fallback (may be NULL); when that
// fails too, returns the defaults. Never returns NULL.
PcSettings *pc_settings_load(const char *path, const char *fallback);

// Writes the settings; errors are ignored (not being able to remember is not worth a message).
void pc_settings_save(const PcSettings *settings, const char *path);

// ~/.config/pcmon/settings.ini and the older ~/.config/pymon/settings.ini (honours
// XDG_CONFIG_HOME).
char *pc_settings_default_path(void);
char *pc_settings_legacy_path(void);

// Replaces or removes (empty name) the name of one fan.
void pc_settings_set_fan_name(PcSettings *settings, const char *fan_id, const char *name);

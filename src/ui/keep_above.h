// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Keeps a window above the others. GTK 4 removed gtk_window_set_keep_above, so on X11 this asks
// the window manager directly (_NET_WM_STATE_ABOVE). There is no equivalent on Wayland.

#pragma once

#include <gtk/gtk.h>

// TRUE when this session can do it (an X11 display and a build that knows X11).
gboolean pc_keep_above_supported(void);

// Turns "always on top" on or off. Does nothing until the window is mapped, and nothing when
// it is not supported; call it again from the window's "map" signal.
void pc_keep_above_set(GtkWindow *window, gboolean on);

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Sends the EWMH _NET_WM_STATE_ABOVE request for a mapped X11 window.

#include "ui/keep_above.h"

#if defined(PCMON_HAVE_X11) && defined(GDK_WINDOWING_X11)
#include <string.h>

#include <gdk/x11/gdkx.h>
// Xlib last: its macros (None, Bool, Status) collide with GTK headers when included earlier.
#include <X11/Xlib.h>

gboolean pc_keep_above_supported(void) {
    GdkDisplay *display = gdk_display_get_default();
    return display != NULL && GDK_IS_X11_DISPLAY(display);
}

void pc_keep_above_set(GtkWindow *window, gboolean on) {
    GdkSurface *surface = gtk_native_get_surface(GTK_NATIVE(window));
    if (surface == NULL || !GDK_IS_X11_SURFACE(surface) ||
        !gtk_widget_get_mapped(GTK_WIDGET(window))) {
        return;
    }
    Display *xdisplay = gdk_x11_display_get_xdisplay(gdk_surface_get_display(surface));
    XEvent event;
    memset(&event, 0, sizeof event);
    event.xclient.type = ClientMessage;
    event.xclient.window = gdk_x11_surface_get_xid(surface);
    event.xclient.message_type = XInternAtom(xdisplay, "_NET_WM_STATE", False);
    event.xclient.format = 32;
    event.xclient.data.l[0] = on ? 1 : 0; // _NET_WM_STATE_ADD or _NET_WM_STATE_REMOVE
    event.xclient.data.l[1] = (long)XInternAtom(xdisplay, "_NET_WM_STATE_ABOVE", False);
    event.xclient.data.l[3] = 1; // the request comes from a normal application
    XSendEvent(xdisplay, DefaultRootWindow(xdisplay), False,
               SubstructureNotifyMask | SubstructureRedirectMask, &event);
    XFlush(xdisplay);
}

#else

gboolean pc_keep_above_supported(void) {
    return FALSE;
}

void pc_keep_above_set(GtkWindow *window, gboolean on) {
    (void)window;
    (void)on;
}

#endif

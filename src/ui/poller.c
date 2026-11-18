// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// The sensor thread, and the hand-over of each snapshot to the main loop.

#include "ui/poller.h"

// Shared by the main thread and the sensor thread; freed when both let go of it.
struct PcPoller {
    gint refs;
    gint stop;          // set (atomically) by pc_poller_stop
    gboolean stopped;   // main thread only: the callback must not run any more
    PcSensors *sensors; // used by the thread only
    double interval;
    PcPollerCallback callback;
    gpointer user_data;
};

typedef struct {
    PcPoller *poller;
    PcSnapshot *snapshot;
} Delivery;

static void poller_unref(PcPoller *p) {
    if (g_atomic_int_dec_and_test(&p->refs)) {
        g_free(p);
    }
}

// Runs on the main thread.
static gboolean deliver(gpointer data) {
    Delivery *d = data;
    if (d->poller->stopped) {
        pc_snapshot_free(d->snapshot);
    } else {
        d->poller->callback(d->snapshot, d->poller->user_data);
    }
    poller_unref(d->poller);
    g_free(d);
    return G_SOURCE_REMOVE;
}

static gpointer poll_thread(gpointer data) {
    PcPoller *p = data;
    while (!g_atomic_int_get(&p->stop)) {
        PcSnapshot *snap = pc_sensors_collect(p->sensors);
        Delivery *d = g_new0(Delivery, 1);
        g_atomic_int_inc(&p->refs);
        d->poller = p;
        d->snapshot = snap;
        g_idle_add(deliver, d);
        // Sleep in short slices so that stopping does not have to wait a whole interval.
        gint64 until = g_get_monotonic_time() + (gint64)(p->interval * G_USEC_PER_SEC);
        while (!g_atomic_int_get(&p->stop) && g_get_monotonic_time() < until) {
            g_usleep(50 * 1000);
        }
    }
    pc_sensors_free(p->sensors);
    poller_unref(p);
    return NULL;
}

PcPoller *pc_poller_start(PcSensors *sensors, double interval_seconds, PcPollerCallback callback,
                          gpointer user_data) {
    PcPoller *p = g_new0(PcPoller, 1);
    p->refs = 2; // one for the caller, one for the thread
    p->sensors = sensors;
    p->interval = interval_seconds;
    p->callback = callback;
    p->user_data = user_data;
    GThread *thread = g_thread_new("pcmon-sensors", poll_thread, p);
    g_thread_unref(thread); // never joined: it ends by itself after pc_poller_stop
    return p;
}

void pc_poller_stop(PcPoller *poller) {
    poller->stopped = TRUE;
    g_atomic_int_set(&poller->stop, 1);
    poller_unref(poller);
}

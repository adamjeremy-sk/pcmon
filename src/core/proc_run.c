// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Adam Hainstock (@adamjeremy-sk)
// Starts a child process, collects its output without blocking forever, and reaps it.

#include "core/proc_run.h"

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

char *pc_run_capture(char *const *argv, int timeout_ms) {
    GPid pid = 0;
    int out_fd = -1;
    if (!g_spawn_async_with_pipes(NULL, (char **)argv, NULL,
                                  G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD |
                                      G_SPAWN_STDERR_TO_DEV_NULL,
                                  NULL, NULL, &pid, NULL, &out_fd, NULL, NULL)) {
        return NULL;
    }
    GString *output = g_string_new(NULL);
    gint64 deadline = g_get_monotonic_time() + (gint64)timeout_ms * 1000;
    gboolean finished = FALSE;
    while (!finished) {
        gint64 left_us = deadline - g_get_monotonic_time();
        if (left_us <= 0) {
            break;
        }
        struct pollfd pfd = {out_fd, POLLIN, 0};
        int ready = poll(&pfd, 1, (int)(left_us / 1000) + 1);
        if (ready < 0 && errno == EINTR) {
            continue;
        }
        if (ready <= 0) {
            break;
        }
        char buffer[4096];
        ssize_t n = read(out_fd, buffer, sizeof buffer);
        if (n > 0) {
            g_string_append_len(output, buffer, n);
        } else if (n == 0) {
            finished = TRUE; // end of output
        } else if (errno != EINTR && errno != EAGAIN) {
            break;
        }
    }
    close(out_fd);
    if (!finished) {
        kill(pid, SIGKILL);
    }
    int status = 0;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
    }
    g_spawn_close_pid(pid);
    if (!finished) {
        g_string_free(output, TRUE);
        return NULL;
    }
    return g_string_free(output, FALSE);
}

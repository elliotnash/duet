#include <stdio.h>
#include <sys/inotify.h>
#include <glib.h>

#include "keyboard.h"
#include "context.h"
#include "rotation.h"

static GMainLoop *loop;

int main() {
    duet_context_t status = { .keyboardConnected = 1, .rotation = ROTATION_LANDSCAPE, .mode = MODE_AUTO };

    keyboard_watch(&status);
    rotation_watch();

    loop = g_main_loop_new(NULL, TRUE);
    g_main_loop_run(loop);

    rotation_cleanup();
    keyboard_cleanup();

    g_main_loop_unref(loop);

    return 0;
}

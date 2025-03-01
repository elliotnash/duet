#include <stdio.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <glib.h>

#include "keyboard.h"
#include "context.h"
#include "rotation.h"

static GMainLoop *loop;

int main() {
    duet_context_t status = { .keyboardConnected = 1, .rotation = ROTATION_LANDSCAPE, .mode = MODE_AUTO };

    int fd = inotify_init();
    if (fd < 0) {
        fprintf(stderr, "Error initing inotify.\n");
        return -1;
    }

    keyboard_watch(fd, &status);
    rotation_watch();

    printf("Is kb connected: %d\n", get_keyboard_status());

    loop = g_main_loop_new(NULL, TRUE);
    g_main_loop_run(loop);

    rotation_cleanup();
    keyboard_cleanup();
    close(fd);

    g_main_loop_unref(loop);

    return 0;
}

#include <glib.h>

#include "context.h"
#include "keyboard.h"
#include "rotation.h"

static GMainLoop *loop;

int main() {
  duet_context_t status = {.keyboardConnected = 1,
                           .rotation = ROTATION_LANDSCAPE,
                           .mode = MODE_AUTO};

  keyboard_watch(&status);
  rotation_watch(&status);

  loop = g_main_loop_new(NULL, TRUE);
  g_main_loop_run(loop);

  rotation_cleanup();
  keyboard_cleanup();

  g_main_loop_unref(loop);

  return 0;
}

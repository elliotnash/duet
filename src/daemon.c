#include <glib.h>
#include <glib-unix.h>
#include <stdio.h>

#include "config.h"
#include "display.h"
#include "command.h"
#include "context.h"
#include "keyboard.h"
#include "rotation.h"
#include "brightness.h"

static gboolean handle_sigint(gpointer data) {
  GMainLoop *loop = data;
  g_main_loop_quit(loop);
  return G_SOURCE_REMOVE;
}

int main() {
  GError *cfg_err = NULL;
  duet_config_t *config = duet_config_load("/etc/duet.ini", &cfg_err);
  if (cfg_err) {
    g_printerr("Failed to load config: %s\n", cfg_err->message);
    g_printerr("Please create a config file at /etc/duet.ini\n");
    g_error_free(cfg_err);
    return 1;
  }

  duet_context_t status = {.keyboardConnected = 1,
                           .rotation = ROTATION_LANDSCAPE,
                           .mode = MODE_AUTO};

  display_set_config(config);

  keyboard_watch(&status);
  rotation_watch(&status);
  command_watch(&status);
  if (config->sync_brightness) {
    brightness_watch(config);
  }

  GMainLoop *loop = g_main_loop_new(NULL, TRUE);
  g_unix_signal_add(SIGINT, handle_sigint, loop);
  g_main_loop_run(loop);

  if (config->sync_brightness) {
    brightness_cleanup();
  }
  command_cleanup();
  rotation_cleanup();
  keyboard_cleanup();

  g_main_loop_unref(loop);

  printf("duet daemon stopped.\n");

  return 0;
}

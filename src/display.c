#include "display.h"
#include "config.h"

#include <glib.h>
#include <stdio.h>



static const duet_config_t *config = NULL;

void display_set_config(const duet_config_t *cfg) {
  config = cfg;
}

void system_fmt(char *format, ...) {
  char command[420];
  va_list args;
  va_start(args, format);
  vsnprintf(command, sizeof(command), format, args);
  system(command);
  va_end(args);
}

static duet_context_t lastContext = {
    .keyboardConnected = -1, .rotation = -1, .mode = -1};
/**
 * Sets the layout for a given keyboard and rotation status
 * @param context The device status.
 */
void setLayout(duet_context_t *context) {
  /* If
      a. the keyboard is connected and was previously connected
      b. the keyboard state, mode, and rotation haven't changed
      c. the keyboard state and mode haven't changed, and the rotation is
  manually selected

  Then there's no need to update display.
  */
  if ((lastContext.keyboardConnected && context->keyboardConnected) ||
      (lastContext.keyboardConnected == context->keyboardConnected &&
       lastContext.mode == context->mode &&
       (lastContext.rotation == context->rotation ||
        context->mode != MODE_AUTO))) {
    lastContext.keyboardConnected = context->keyboardConnected;
    lastContext.mode = context->mode;
    lastContext.rotation = context->rotation;
    return;
  }
  lastContext.keyboardConnected = context->keyboardConnected;
  lastContext.mode = context->mode;
  lastContext.rotation = context->rotation;

  g_print("Updating layout - keyboard: %d, rotation: %d, mode: %d\n",
          context->keyboardConnected, context->rotation, context->mode);

  // If keyboard not connected, always show only one monitor.
  if (context->keyboardConnected) {
    setSingleMonitor();
  } else if (context->mode == MODE_AUTO) {
    // Auto mode, set the layout for the passed rotation. If rotation not
    // recognized do nothing
    if (context->rotation == ROTATION_PORTRAIT_90) {
      setPortrait90();
    } else if (context->rotation == ROTATION_PORTRAIT_270) {
      setPortrait270();
    } else if (context->rotation == ROTATION_LANDSCAPE) {
      setLandscape();
    }
  } else if (context->mode == MODE_MIRROR) {
    setMirror();
  } else if (context->mode == MODE_LANDSCAPE) {
    setLandscape();
  } else if (context->mode == MODE_PORTRAIT_90) {
    setPortrait90();
  } else if (context->mode == MODE_PORTRAIT_270) {
    setPortrait270();
  }
}

// Mirrors the top display and bottom such that the top is flipped 180 (to be
// someone accross a table).
void setMirror() {
  system(config->mirror_command);
}

// Disables the monitor under the keyboard
void setSingleMonitor() {
  system(config->single_monitor_command);
}

// Both monitors enabled in landscape mode (stacked vertically)
void setLandscape() {
  system(config->landscape_command);
}

// Both monitors enabled in portrait mode, such that the primary monitor is on
// the right and the keyboard side display is on the left (90 deg clockwise).
void setPortrait90() {
  system(config->portrait_right_command);
}

// Both monitors enabled in portrait mode, such that the primary monitor is on
// the left and the keyboard side display is on the right (90 deg
// counterclockwise).
void setPortrait270() {
  system(config->portrait_left_command);
}

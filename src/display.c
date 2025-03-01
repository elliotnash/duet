#include <stdio.h>
#include <glib.h>

#include "display.h"

// TODO move some of this to a config (json?)

static const char* primaryMonitor = "eDP-1";
static const char* secondaryMonitor = "eDP-2";

static const int internalHeight = 1800;
static const double internalScale = 1.25;
static const int internalLogicalHeight = internalHeight / internalScale;

static const int vrr = 1;

void system_fmt(char* format, ...) {
    char command[420];
    va_list args;
    va_start(args, format);
    vsnprintf(command, sizeof(command), format, args);
    system(command);
    va_end(args);
}

static duet_context_t lastContext = { .keyboardConnected = -1, .rotation = -1, .mode = -1 };
/** 
 * Sets the layout for a given keyboard and rotation status
 * @param context The device status.
 */
void setLayout(duet_context_t *context) {
    /* If 
        a. the keyboard is connected and was previously connected
        b. the keyboard state, mode, and rotation haven't changed
        c. the keyboard state and mode haven't changed, and the rotation is manually selected

    Then there's no need to update display.
    */
    if (
        (lastContext.keyboardConnected && context->keyboardConnected)
        || (lastContext.keyboardConnected == context->keyboardConnected 
            && lastContext.mode == context->mode 
            && (lastContext.rotation == context->rotation || context->mode != MODE_AUTO)
        )
    ) {
        lastContext.keyboardConnected = context->keyboardConnected;
        lastContext.mode = context->mode;
        lastContext.rotation = context->rotation;
        return;
    }
    lastContext.keyboardConnected = context->keyboardConnected;
    lastContext.mode = context->mode;
    lastContext.rotation = context->rotation;

    g_print("Updating layout - keyboard: %d, rotation: %d, mode: %d\n", context->keyboardConnected, context->rotation, context->mode);

    // If keyboard not connected, always show only one monitor.
    if (context->keyboardConnected) {
        setSingleMonitor();
    } else if (context->mode == MODE_AUTO) {
        // Auto mode, set the layout for the passed rotation. If rotation not recognized do nothing
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

    // Restart ags, required due to https://github.com/end-4/dots-hyprland/issues/791
    system("agsv1 -q; swww kill; agsv1 & disown & swww-daemon & disown");
}

// Mirrors the top display and bottom such that the top is flipped 180 (to be someone accross a table).
void setMirror() {
    system_fmt("hyprctl --batch \"keyword monitor %s,preferred,0x0,%f,transform,2,vrr,%d ; keyword monitor %s,preferred,0x0,%f,mirror,%s,vrr,%d \"", primaryMonitor, internalScale, vrr, secondaryMonitor, internalScale, primaryMonitor, vrr);
}

// Disables the monitor under the keyboard
void setSingleMonitor() {
    system_fmt("hyprctl --batch \"keyword monitor %s,preferred,0x0,%f,vrr,%d ; keyword monitor %s,disabled \"", primaryMonitor, internalScale, vrr, secondaryMonitor);
}

// Both monitors enabled in landscape mode (stacked vertically)
void setLandscape() {
    system_fmt("hyprctl --batch \"keyword monitor %s,preferred,0x0,%f,vrr,%d ; keyword monitor %s,preferred,0x%d,%f,vrr,%d \"", primaryMonitor, internalScale, vrr, secondaryMonitor, internalLogicalHeight, internalScale, vrr);
}

// Both monitors enabled in portrait mode, such that the primary monitor is on the right and the keyboard side display is on the left (90 deg clockwise).
void setPortrait90() {
    system_fmt("hyprctl --batch \"keyword monitor %s,preferred,%dx0,%f,transform,1,vrr,%d ; keyword monitor %s,preferred,0x0,%f,transform,1,vrr,%d \"", primaryMonitor, internalLogicalHeight, internalScale, vrr, secondaryMonitor, internalScale, vrr);
}

// Both monitors enabled in portrait mode, such that the primary monitor is on the left and the keyboard side display is on the right (90 deg counterclockwise).
void setPortrait270() {
    system_fmt("hyprctl --batch \"keyword monitor %s,preferred,0x0,%f,transform,3,vrr,%d ; keyword monitor %s,preferred,%dx0,%f,transform,3,vrr,%d \"", primaryMonitor, internalScale, vrr, secondaryMonitor, internalLogicalHeight, internalScale, vrr);
}

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

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

static int lastKeyboardConnected, lastRotation, lastMode = -1;
/** 
 * Sets the layout for a given keyboard and rotation status
 * 
 * @param keyboardConnected Whether the keyboard is corrected
 * 
 * @param rotation The device rotation:
 *      0 = landscape,
 *      1 = portrait 90deg,
 *      2 = portrait -90deg
 * 
 * @param mode The display mode:
 *      0 = auto,
 *      1 = mirror,
 *      2 = landscape,
 *      3 = portrait 90deg,
 *      4 = portrait -90deg
 */
void setLayout(int keyboardConnected, int rotation, int mode) {
    // If nothings changed no need to update (we only care about rotation in auto.).
    if (lastKeyboardConnected == keyboardConnected && lastMode == mode && (lastRotation == rotation || mode != MODE_AUTO)) {
        lastRotation = rotation;
        return;
    }
    lastKeyboardConnected = keyboardConnected;
    lastMode = mode;
    lastRotation = rotation;

    // If keyboard not connected, always show only one monitor.
    if (keyboardConnected) {
        setSingleMonitor();
    } else if (mode == MODE_AUTO) {
        // Auto mode, set the layout for the passed rotation. If rotation not recognized do nothing
        if (rotation == ROTATION_PORTRAIT_90) {
            setPortrait90();
        } else if (rotation == ROTATION_PORTRAIT_270) {
            setPortrait270();
        } else if (rotation == ROTATION_LANDSCAPE) {
            setLandscape();
        }
    } else if (mode == MODE_MIRROR) {
        setMirror();
    } else if (mode == MODE_LANDSCAPE) {
        setLandscape();
    } else if (mode == MODE_PORTRAIT_90) {
        setPortrait90();
    } else if (mode == MODE_PORTRAIT_270) {
        setPortrait270();
    }

    // Restart ags, required due to https://github.com/end-4/dots-hyprland/issues/791
    system("agsv1 -q; swww kill; agsv1 & swww-daemon & disown");
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

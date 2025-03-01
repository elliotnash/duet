#ifndef CONTEXT_H
#define CONTEXT_H

#define MODE_AUTO 0
#define MODE_MIRROR 1
#define MODE_LANDSCAPE 2
#define MODE_PORTRAIT_90 3
#define MODE_PORTRAIT_270 4

#define ROTATION_LANDSCAPE 0
#define ROTATION_PORTRAIT_90 1
#define ROTATION_PORTRAIT_270 2

struct DuetContext {
    /** Whether the keyboard is connected */
    int keyboardConnected;
    /**
     * rotation The device rotation:
     *      0 = landscape,
     *      1 = portrait 90deg,
     *      2 = portrait -90deg
     */
    int rotation;
    /** 
     * The display mode:
     *      0 = auto,
     *      1 = mirror,
     *      2 = landscape,
     *      3 = portrait 90deg,
     *      4 = portrait -90deg
     */
    int mode;
} typedef duet_context_t;

#endif
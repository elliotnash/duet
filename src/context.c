#include <glib.h>

#include "context.h"

int parse_orientation(const gchar *orientation_str) {
    if (g_strcmp0(orientation_str, "normal") == 0)
        return ROTATION_LANDSCAPE;
    if (g_strcmp0(orientation_str, "left-up") == 0)
        return ROTATION_PORTRAIT_90;
    if (g_strcmp0(orientation_str, "right-up") == 0)
        return ROTATION_PORTRAIT_270;

    return -1;
}

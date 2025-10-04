#pragma once

#include <glib.h>
#include "config.h"

// Initialize brightness sync service
// Returns TRUE on success, FALSE on failure
gboolean brightness_watch(duet_config_t *cfg);

// Cleanup brightness sync service
void brightness_cleanup(void);

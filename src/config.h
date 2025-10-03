#pragma once

#include <glib.h>

typedef struct duet_config_s {
	// Layout commands (group: [Layout Commands])
	gchar *single_monitor_command;
	gchar *mirror_command;
	gchar *landscape_command;
	gchar *portrait_right_command;
	gchar *portrait_left_command;
} duet_config_t;

// Loads configuration from `config.ini` adjacent to the executable working directory
// or from the provided absolute path if not NULL. Returns newly allocated config
// on success, or NULL on failure. Caller must free with duet_config_free.
duet_config_t *duet_config_load(const gchar *config_path, GError **error);

// Frees all memory associated with duet_config_t
void duet_config_free(duet_config_t *config);



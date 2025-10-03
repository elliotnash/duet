// Configuration loader using GLib GKeyFile
#include "config.h"

#define GROUP_LAYOUT "Layout Commands"

static gchar *dup_key_string(GKeyFile *kf, const gchar *group, const gchar *key) {
	GError *error = NULL;
	gchar *value = g_key_file_get_string(kf, group, key, &error);
	if (!value) {
		if (error) g_error_free(error);
	}
	return value; // may be NULL
}

static void set_error_missing(GError **error, const gchar *key) {
	if (!error) return;
	g_set_error(error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND,
	           "Missing required key: %s in group [%s]", key, GROUP_LAYOUT);
}

duet_config_t *duet_config_load(const gchar *config_path, GError **error) {
	GError *local_error = NULL;
	GKeyFile *key_file = g_key_file_new();
	if (!key_file) {
		g_set_error(error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_PARSE,
		           "Failed to allocate GKeyFile");
		return NULL;
	}

	// Determine path
	const gchar *path = config_path ? config_path : "config.ini";
	if (!g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, &local_error)) {
		if (error) *error = local_error; else if (local_error) g_error_free(local_error);
		g_key_file_unref(key_file);
		return NULL;
	}

	duet_config_t *cfg = g_new0(duet_config_t, 1);
	if (!cfg) {
		g_key_file_unref(key_file);
		g_set_error(error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_PARSE,
		           "Failed to allocate config struct");
		return NULL;
	}

	// Required commands under [Layout Commands]
	if (!g_key_file_has_group(key_file, GROUP_LAYOUT)) {
		g_set_error(error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_GROUP_NOT_FOUND,
		           "Missing required group: [%s]", GROUP_LAYOUT);
		duet_config_free(cfg);
		g_key_file_unref(key_file);
		return NULL;
	}

	cfg->single_monitor_command = dup_key_string(key_file, GROUP_LAYOUT, "SINGLE_MONITOR_COMMAND");
	cfg->mirror_command = dup_key_string(key_file, GROUP_LAYOUT, "MIRROR_COMMAND");
	cfg->landscape_command = dup_key_string(key_file, GROUP_LAYOUT, "LANDSCAPE_COMMAND");
	cfg->portrait_right_command = dup_key_string(key_file, GROUP_LAYOUT, "PORTRAIT_RIGHT_COMMAND");
	cfg->portrait_left_command = dup_key_string(key_file, GROUP_LAYOUT, "PORTRAIT_LEFT_COMMAND");

	// Validate required
	if (!cfg->single_monitor_command) { set_error_missing(error, "SINGLE_MONITOR_COMMAND"); goto fail; }
	if (!cfg->mirror_command) { set_error_missing(error, "MIRROR_COMMAND"); goto fail; }
	if (!cfg->landscape_command) { set_error_missing(error, "LANDSCAPE_COMMAND"); goto fail; }
	if (!cfg->portrait_right_command) { set_error_missing(error, "PORTRAIT_RIGHT_COMMAND"); goto fail; }
	if (!cfg->portrait_left_command) { set_error_missing(error, "PORTRAIT_LEFT_COMMAND"); goto fail; }

	g_key_file_unref(key_file);
	return cfg;

fail:
	duet_config_free(cfg);
	g_key_file_unref(key_file);
	return NULL;
}

void duet_config_free(duet_config_t *config) {
	if (!config) return;
	g_free(config->single_monitor_command);
	g_free(config->mirror_command);
	g_free(config->landscape_command);
	g_free(config->portrait_right_command);
	g_free(config->portrait_left_command);
	g_free(config);
}

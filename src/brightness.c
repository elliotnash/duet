#include "brightness.h"
#include "config.h"

#include <glib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/inotify.h>

static GIOChannel *inotify_channel = NULL;
static gint inotify_fd = -1;
static gint target_fd = -1;
static gchar *last_brightness = NULL;
static const duet_config_t *config = NULL;

// Read current brightness value from source file
static gchar *read_brightness(void) {
    gchar *content = NULL;
    GError *error = NULL;
    
    if (!config || !config->source_display) {
        g_printerr("No config or source display path available\n");
        return NULL;
    }
    
    if (!g_file_get_contents(config->source_display, &content, NULL, &error)) {
        g_printerr("Failed to read brightness: %s\n", error->message);
        g_error_free(error);
        return NULL;
    }
    
    // Remove trailing newline if present
    g_strchomp(content);
    return content;
}

// Write brightness value to target file
static gboolean write_brightness(const gchar *brightness) {
    if (!brightness) return FALSE;
    
    if (!config || !config->target_display) {
        g_printerr("No config or target display path available\n");
        return FALSE;
    }
    
    if (target_fd == -1) {
        target_fd = open(config->target_display, O_WRONLY);
        if (target_fd == -1) {
            g_printerr("Failed to open target brightness file: %s\n", g_strerror(errno));
            return FALSE;
        }
    }
    
    // Write the brightness value
    ssize_t written = write(target_fd, brightness, strlen(brightness));
    if (written == -1) {
        g_printerr("Failed to write brightness: %s\n", g_strerror(errno));
        return FALSE;
    }
    
    return TRUE;
}

// Inotify event callback
static gboolean inotify_event(GIOChannel *source, GIOCondition condition, gpointer data) {
    if (condition & G_IO_IN) {
        char buffer[4096];
        ssize_t length = read(inotify_fd, buffer, sizeof(buffer));
        
        if (length > 0) {
            struct inotify_event *event = (struct inotify_event *)buffer;
            
            // Check if this is a modify event
            if (event->mask & IN_MODIFY) {
                gchar *current_brightness = read_brightness();
                if (!current_brightness) {
                    return G_SOURCE_CONTINUE;
                }
                
                // Check if brightness actually changed
                if (!last_brightness || g_strcmp0(last_brightness, current_brightness) != 0) {
                    g_free(last_brightness);
                    last_brightness = current_brightness;
                    
                    g_print("Brightness changed to: %s\n", current_brightness);
                    
                    if (write_brightness(current_brightness)) {
                        g_print("Synced brightness to intel backlight\n");
                    } else {
                        g_printerr("Failed to sync brightness\n");
                    }
                } else {
                    g_free(current_brightness);
                }
            }
        }
    }
    
    return G_SOURCE_CONTINUE;
}

gboolean brightness_watch(duet_config_t *cfg) {
    // Get config from display module
    config = cfg;
    if (!config) {
        g_printerr("No config available for brightness sync\n");
        return FALSE;
    }
    
    // Check if source file exists
    if (!g_file_test(config->source_display, G_FILE_TEST_EXISTS)) {
        g_printerr("Source brightness file does not exist: %s\n", config->source_display);
        return FALSE;
    }
    
    // Check if target file exists
    if (!g_file_test(config->target_display, G_FILE_TEST_EXISTS)) {
        g_printerr("Target brightness file does not exist: %s\n", config->target_display);
        return FALSE;
    }
    
    // Initialize inotify
    inotify_fd = inotify_init();
    if (inotify_fd == -1) {
        g_printerr("Failed to initialize inotify: %s\n", g_strerror(errno));
        return FALSE;
    }
    
    // Add watch for the brightness file
    int watch_fd = inotify_add_watch(inotify_fd, config->source_display, IN_MODIFY);
    if (watch_fd == -1) {
        g_printerr("Failed to add inotify watch: %s\n", g_strerror(errno));
        close(inotify_fd);
        inotify_fd = -1;
        return FALSE;
    }
    
    // Create GIOChannel for inotify events
    inotify_channel = g_io_channel_unix_new(inotify_fd);
    if (!inotify_channel) {
        g_printerr("Failed to create GIOChannel for inotify\n");
        inotify_rm_watch(inotify_fd, watch_fd);
        close(inotify_fd);
        inotify_fd = -1;
        return FALSE;
    }
    
    g_io_channel_set_encoding(inotify_channel, NULL, NULL);
    g_io_channel_set_close_on_unref(inotify_channel, TRUE);
    
    // Add watch for inotify events
    g_io_add_watch(inotify_channel, G_IO_IN, inotify_event, NULL);
    
    // Read initial brightness value
    last_brightness = read_brightness();
    if (last_brightness) {
        g_print("Initial brightness: %s\n", last_brightness);
        // Sync initial value
        write_brightness(last_brightness);
    }
    
    g_print("Brightness sync service started (using inotify)\n");
    return TRUE;
}

void brightness_cleanup(void) {
    g_print("Cleaning up brightness sync service\n");
    
    if (inotify_channel) {
        g_io_channel_unref(inotify_channel);
        inotify_channel = NULL;
    }
    
    if (inotify_fd != -1) {
        close(inotify_fd);
        inotify_fd = -1;
    }
    
    if (target_fd != -1) {
        close(target_fd);
        target_fd = -1;
    }
    
    g_free(last_brightness);
    last_brightness = NULL;
}

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#include "display.h"
#include "command.h"

static int server_fd = -1;
static char socket_path[1024];
static GIOChannel *server_channel = NULL;

static void mode_switch(duet_context_t *context, char *payload) {
    int mode = atoi(payload);
    printf("Received mode switch: %d, old mode: %d\n", mode, context->mode);
    context->mode = mode;
    setLayout(context);
}

static void message_received(duet_context_t *context, const char *event, size_t len) {
    // Split first colon, before is event type, after is the payload
    char* payload = strchr(event, ':');
    if (payload == NULL) {
        return;
    }
    *payload = '\0';
    ++payload;
    *(payload + strlen(payload) - 1) = '\0';

    if (g_str_equal(event, "mode")) {
        mode_switch(context, payload);
    } else {
        printf("Unknown event: %s\n", event);
    }
}

// Client connection callback
static gboolean client_data_cb(GIOChannel *source, GIOCondition condition, gpointer data) {
    duet_context_t *context = (duet_context_t *) data;

    if (condition & G_IO_HUP) {
        // Client disconnected
        g_io_channel_shutdown(source, TRUE, NULL);
        g_io_channel_unref(source);
        return G_SOURCE_REMOVE; // Remove the source
    }

    if (condition & G_IO_IN) {
        char buffer[4096];
        gsize bytes_read;
        GError *error = NULL;

        GIOStatus status = g_io_channel_read_chars(source, buffer, sizeof(buffer), &bytes_read, &error);
        if (status == G_IO_STATUS_ERROR) {
            g_error_free(error);
            g_io_channel_shutdown(source, TRUE, NULL);
            g_io_channel_unref(source);
            return G_SOURCE_REMOVE;
        }

        if (bytes_read > 0) {
            message_received(context, buffer, bytes_read);
        }
    }

    return G_SOURCE_CONTINUE;
}

// Server callback to accept new connections
static gboolean server_conn_cb(GIOChannel *source, GIOCondition condition, gpointer data) {
    duet_context_t *context = (duet_context_t *) data;

    if (condition & G_IO_IN) {
        int client_fd = accept(g_io_channel_unix_get_fd(source), NULL, NULL);
        if (client_fd == -1) {
            perror("accept");
            return G_SOURCE_CONTINUE;
        }

        // Set client socket to non-blocking
        int flags = fcntl(client_fd, F_GETFL, 0);
        if (flags == -1 || fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
            perror("fcntl");
            close(client_fd);
            return G_SOURCE_CONTINUE;
        }

        GIOChannel *client_channel = g_io_channel_unix_new(client_fd);
        g_io_channel_set_encoding(client_channel, NULL, NULL);
        g_io_channel_set_close_on_unref(client_channel, TRUE);

        // Watch for client data and hangups
        g_io_add_watch(client_channel, G_IO_IN | G_IO_HUP, client_data_cb, context);
    }

    return G_SOURCE_CONTINUE;
}

void command_watch(duet_context_t *context) {
    const char *runtime_dir = g_getenv("XDG_RUNTIME_DIR");
    if (!runtime_dir) {
        g_printerr("XDG_RUNTIME_DIR is not set\n");
        return;
    }

    // Build full socket path in one step
    int path_len = snprintf(
        socket_path, 
        sizeof(socket_path), 
        "%s/duet/cmd.socket", 
        runtime_dir
    );

    // Check for truncation
    if (path_len < 0) {
        perror("snprintf");
        return;
    } else if (path_len >= sizeof(socket_path)) {
        g_printerr("Socket path exceeds buffer size\n");
        return;
    }

    // Extract directory part using GLib
    gchar *dir_path = g_path_get_dirname(socket_path);
    if (!dir_path) {
        g_printerr("Failed to parse directory\n");
        return;
    }

    // Create directory (with proper permissions)
    if (mkdir(dir_path, 0700) == -1 && errno != EEXIST) {
        perror("mkdir");
        g_free(dir_path);
        return;
    }
    g_free(dir_path);

    // Remove existing socket file
    unlink(socket_path);

    // Create socket
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return;
    }

    // Bind socket
    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(server_fd);
        return;
    }

    // Listen
    if (listen(server_fd, SOMAXCONN) == -1) {
        perror("listen");
        close(server_fd);
        return;
    }

    // Non-blocking mode
    int flags = fcntl(server_fd, F_GETFL, 0);
    if (flags == -1 || fcntl(server_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
        close(server_fd);
        return;
    }

    // Add to GLib main loop
    server_channel = g_io_channel_unix_new(server_fd);
    g_io_channel_set_encoding(server_channel, NULL, NULL);
    g_io_channel_set_close_on_unref(server_channel, TRUE);
    g_io_add_watch(server_channel, G_IO_IN, server_conn_cb, context);
}

void command_cleanup() {
    printf("Cleaning up commands\n");

    g_io_channel_unref(server_channel);
    unlink(socket_path);
}

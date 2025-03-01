#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <gio/gio.h>

#include "keyboard.h"
#include "display.h"

const uint16_t keyboardVendorId = 0x0b05;
const uint16_t keyboardProductId = 0x1b2c;

static const char* watchBaseDir = "/dev/bus/usb";

static guint inotify_watch_id = 0;
static libusb_context *ctx;

/**
 * Checks if the zenbook duo keyboard is attached by POGO.
 * 
 * @returns -1 if error fetching status, 0 if keyboard is disconnected, or 1 if keyboard is connected.
 */
int get_keyboard_status() {
    libusb_device **devs;

    ssize_t count = libusb_get_device_list(ctx, &devs);
    if (count < 0) {
        fprintf(stderr, "Error getting device list: %s\n", libusb_error_name(count));
        return -1;
    }

    int connected = 0;
	 
    for (ssize_t i = 0; i < count; i++) {
        libusb_device *dev = devs[i];
        struct libusb_device_descriptor desc;
        int res = libusb_get_device_descriptor(dev, &desc);
        if (res < 0) {
            fprintf(stderr, "Error getting device descriptor: %s\n", libusb_error_name(res));
            continue;
        }
        if (desc.idVendor == keyboardVendorId && desc.idProduct == keyboardProductId) {
            connected = 1;
            break;
        }
    }
    
    libusb_free_device_list(devs, 1);

    return connected;
}

static gboolean handle_inotify_event(GIOChannel *source, GIOCondition condition, gpointer data) {
    duet_context_t *context = (duet_context_t *) data;

    char buffer[IN_BUFF_SIZE];
    gsize bytes_read;
    GError *error = NULL;
    
    GIOStatus status = g_io_channel_read_chars(source, buffer, sizeof(buffer), &bytes_read, &error);
    
    if (status == G_IO_STATUS_ERROR) {
        g_warning("Inotify read error: %s", error->message);
        g_error_free(error);
        return G_SOURCE_REMOVE;
    }

    if (bytes_read == 0)
        return G_SOURCE_CONTINUE;

    for (char *ptr = buffer; ptr < buffer + bytes_read;) {
        struct inotify_event *event = (struct inotify_event*)ptr;
        
        if (event->len > 0) {
            g_usleep(1000000);
            int kb = get_keyboard_status();
            if (kb != -1) {
                printf("KB status updated: %d\n", kb);
                context->keyboardConnected = kb;
                // setLayout(context);
            }
        }

        ptr += sizeof(struct inotify_event) + event->len;
    }

    return G_SOURCE_CONTINUE;
}

void keyboard_watch(int fd, duet_context_t *status) {
    int res = libusb_init(&ctx);
    if (res < 0) {
        fprintf(stderr, "Error initializing libusb: %s\n", libusb_error_name(res));
        return;
    }

    struct dirent* dent;

    DIR* srcdir = opendir(watchBaseDir);
    if (srcdir == NULL) {
        fprintf(stderr, "Error opening /dev/bus/usb.\n");
        return;
    }

    while((dent = readdir(srcdir)) != NULL) {
        struct stat st;

        if(strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0)
            continue;

        if (fstatat(dirfd(srcdir), dent->d_name, &st, 0) < 0)
        {
            perror(dent->d_name);
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            char* watchDir = malloc((strlen(watchBaseDir) + 1 + strlen(dent->d_name)) * sizeof(char));
            sprintf(watchDir, "%s/%s", watchBaseDir, dent->d_name);
            
            int wd = inotify_add_watch(fd, watchDir, IN_CREATE | IN_DELETE);
            if (wd < 0) {
                fprintf(stderr, "Error adding inotify watch.\n");
            }
        }
    }

    GIOChannel *channel = g_io_channel_unix_new(fd);
    g_io_channel_set_encoding(channel, NULL, NULL);
    g_io_channel_set_buffered(channel, FALSE);

    inotify_watch_id = g_io_add_watch(channel, G_IO_IN | G_IO_HUP | G_IO_ERR, handle_inotify_event, status);

    printf("Watches successfully established.\n");
}

void keyboard_cleanup() {
    if (inotify_watch_id > 0) {
        g_source_remove(inotify_watch_id);
    }

    libusb_exit(ctx);
}

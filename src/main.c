#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/inotify.h>
#include "display.h"

#define IN_BUFF_SIZE 16384

const uint16_t keyboardVendorId = 0x0b05;
const uint16_t keyboardProductId = 0x1b2c;

const char* watchBaseDir = "/dev/bus/usb";

void onKeyboardConnect() {
    printf("Keyboard connected\n");
}

void onKeyboardDisconnect() {
    printf("Keyboard disconnected\n");
}

int isKeyboardConnected = -1;
void checkKeyboardConnected() {
    libusb_device **devs;
    libusb_context *ctx;
    
    int res = libusb_init(&ctx);
    if (res < 0) {
        fprintf(stderr, "Error initializing libusb: %s\n", libusb_error_name(res));
        return;
    }

    ssize_t count = libusb_get_device_list(ctx, &devs);
    if (count < 0) {
        fprintf(stderr, "Error getting device list: %s\n", libusb_error_name(count));
        libusb_exit(ctx);
        return;
    }

    int connected = 0;
	 
    for (ssize_t i = 0; i < count; i++) {
        libusb_device *dev = devs[i];
        struct libusb_device_descriptor desc;
        res = libusb_get_device_descriptor(dev, &desc);
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
    libusb_exit(ctx);

    isKeyboardConnected = connected;
    setLayout(connected, 0, 0);

    if (connected != isKeyboardConnected) {
        // Then our connection status has changed
        isKeyboardConnected = connected;
        if (connected) {
            onKeyboardConnect();
        } else {
            onKeyboardDisconnect();
        }
    }
}

void addWatches(int fd) {
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
                //TODO handle this
                fprintf(stderr, "Error adding inotify watch.\n");
                return;
            }
        }
    }

    printf("Watches successfully established.\n");
}

void watchDevices() {
    int fd = inotify_init();
    if (fd < 0) {
        //TODO handle this
        fprintf(stderr, "Error initing inotify.\n");
        return;
    }

    addWatches(fd);

    int size;
    char buf[IN_BUFF_SIZE];
    while(1) {
        size = read(fd, buf, sizeof(buf));
        if (size <= 0) {
            fprintf(stderr, "Error during inotify read.\n");
            return;
        }

        for (int i = 0; i < size;) {
            struct inotify_event *ev = (struct inotify_event*) &buf[i];

            if (ev->len) {
                checkKeyboardConnected();
            } else {
                fprintf(stderr, "Failed to process inotify event.\n");
                return;
            }

            i += sizeof(struct inotify_event) + ev->len;
        }
    }
}

int main() {
    checkKeyboardConnected();
    watchDevices();

    return 0;
}

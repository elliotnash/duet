#include <gio/gio.h>
#include <libudev.h>
#include <fcntl.h>

#include "keyboard.h"
#include "display.h"

const char* keyboardVendorId = "0b05";
const char* keyboardProductId = "1b2c";

typedef struct {
    char *devpath;
    char *vendor_id;
    char *product_id;
} device_info_t;

typedef struct {
    struct udev_monitor *monitor;
    struct udev *udev_ctx;
    GHashTable *devices;
    duet_context_t *context;
} keyboard_context_t;

static keyboard_context_t kb_context;

static void free_device_info(gpointer data) {
    device_info_t *info = (device_info_t *) data;
    g_free(info->devpath);
    g_free(info->vendor_id);
    g_free(info->product_id);
    g_free(info);
}

static gboolean is_target_device(struct udev_device *dev) {
    const char *vendor = udev_device_get_sysattr_value(dev, "idVendor");
    const char *product = udev_device_get_sysattr_value(dev, "idProduct");
    return (vendor && product && 
            g_str_equal(vendor, keyboardVendorId) &&
            g_str_equal(product, keyboardProductId));
}

static gboolean udev_event(GIOChannel *source, GIOCondition condition, gpointer data) {
    keyboard_context_t *context = (keyboard_context_t *) data;
    struct udev_device *dev = udev_monitor_receive_device(context->monitor);

    if (dev) {
        const char *action = udev_device_get_action(dev);
        const char *devpath = udev_device_get_devpath(dev);

        if (action && devpath) {
            if (g_str_equal(action, "add") && is_target_device(dev)) {
                // Store device details
                device_info_t *info = g_new0(device_info_t, 1);
                info->devpath = g_strdup(devpath);
                info->vendor_id = g_strdup(udev_device_get_sysattr_value(dev, "idVendor"));
                info->product_id = g_strdup(udev_device_get_sysattr_value(dev, "idProduct"));
                g_hash_table_insert(context->devices, info->devpath, info);

                g_print("Keyboard CONNECTED: %s (Vendor: %s, Product: %s)\n",
                       devpath, info->vendor_id, info->product_id);
                context->context->keyboardConnected = TRUE;
                setLayout(context->context);
            } else if (g_str_equal(action, "remove")) {
                // Retrieve stored details
                device_info_t *info = g_hash_table_lookup(context->devices, devpath);
                if (info) {
                    g_print("Keyboard DISCONNECTED: %s (Vendor: %s, Product: %s)\n",
                           devpath, info->vendor_id, info->product_id);
                    g_hash_table_remove(context->devices, devpath);
                    context->context->keyboardConnected = FALSE;
                    setLayout(context->context);
                }
            }
        }
        udev_device_unref(dev);
    }

    return G_SOURCE_CONTINUE;
}

// Check for existing devices on startup
static void check_initial_devices(keyboard_context_t *context) {
    struct udev_enumerate *enumerate = udev_enumerate_new(context->udev_ctx);
    udev_enumerate_add_match_subsystem(enumerate, "usb");
    udev_enumerate_add_match_property(enumerate, "DEVTYPE", "usb_device");
    udev_enumerate_scan_devices(enumerate);

    struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry *entry;

    int connected = FALSE;

    udev_list_entry_foreach(entry, devices) {
        const char *path = udev_list_entry_get_name(entry);
        struct udev_device *dev = udev_device_new_from_syspath(context->udev_ctx, path);

        if (is_target_device(dev)) {
            const char *devpath = udev_device_get_devpath(dev);
            device_info_t *info = g_new0(device_info_t, 1);
            info->devpath = g_strdup(devpath);
            info->vendor_id = g_strdup(udev_device_get_sysattr_value(dev, "idVendor"));
            info->product_id = g_strdup(udev_device_get_sysattr_value(dev, "idProduct"));
            g_hash_table_insert(context->devices, info->devpath, info);

            g_print("Keyboard already CONNECTED: %s (Vendor: %s, Product: %s)\n",
                   devpath, info->vendor_id, info->product_id);
            connected = TRUE;
        }
        udev_device_unref(dev);
    }
    udev_enumerate_unref(enumerate);

    context->context->keyboardConnected = connected;
}

void keyboard_watch(duet_context_t *context) {
    kb_context.context = context;
    kb_context.devices = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, free_device_info);

    kb_context.udev_ctx = udev_new();
    kb_context.monitor = udev_monitor_new_from_netlink(kb_context.udev_ctx, "udev");
    udev_monitor_filter_add_match_subsystem_devtype(kb_context.monitor, "usb", "usb_device");
    udev_monitor_enable_receiving(kb_context.monitor);

    int fd = udev_monitor_get_fd(kb_context.monitor);
    fcntl(fd, F_SETFL, O_NONBLOCK);

    GIOChannel *channel = g_io_channel_unix_new(fd);
    g_io_add_watch(channel, G_IO_IN, udev_event, &kb_context);

    check_initial_devices(&kb_context);
}

void keyboard_cleanup() {
    udev_monitor_unref(kb_context.monitor);
    udev_unref(kb_context.udev_ctx);
    g_hash_table_destroy(kb_context.devices);
}

#include <gio/gio.h>

static GMainLoop *loop;
static guint watch_id;
static GDBusProxy *iio_proxy;
 
 static void
 properties_changed (GDBusProxy *proxy,
                     GVariant   *changed_properties,
                     GStrv       invalidated_properties,
                     gpointer    user_data)
 {
     GVariantDict dict;
     g_variant_dict_init (&dict, changed_properties);
 
     if (g_variant_dict_contains (&dict, "AccelerometerOrientation")) {
         GVariant *val = g_variant_dict_lookup_value(&dict, "AccelerometerOrientation", G_VARIANT_TYPE_STRING);
         if (val) {
             g_print("Orientation changed: %s\n", g_variant_get_string(val, NULL));
             g_variant_unref(val);
         }
     }
 
     g_variant_dict_clear (&dict);
 }
 
 static void
 print_initial_values (void)
 {
     GVariant *v = g_dbus_proxy_get_cached_property(iio_proxy, "AccelerometerOrientation");
     if (v) {
         g_print("Initial orientation: %s\n", g_variant_get_string(v, NULL));
         g_variant_unref(v);
     } else {
         g_print("No accelerometer available\n");
     }
 }
 
 static void
 proxy_connected (GDBusConnection *connection,
              const gchar     *name,
              const gchar     *name_owner,
              gpointer         user_data)
 {
     GError *error = NULL;
     GVariant *ret = NULL;
 
     g_print("Sensor proxy connected\n");
 
     iio_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                             G_DBUS_PROXY_FLAGS_NONE,
                                             NULL,
                                             "net.hadess.SensorProxy",
                                             "/net/hadess/SensorProxy",
                                             "net.hadess.SensorProxy",
                                             NULL, &error);
     
     if (!iio_proxy) {
         g_warning("Failed to create proxy: %s", error->message);
         g_error_free(error);
         g_main_loop_quit(loop);
         return;
     }
 
     g_signal_connect(iio_proxy, "g-properties-changed",
                     G_CALLBACK(properties_changed), NULL);
 
     ret = g_dbus_proxy_call_sync(iio_proxy,
                                 "ClaimAccelerometer",
                                 NULL,
                                 G_DBUS_CALL_FLAGS_NONE,
                                 -1,
                                 NULL, &error);
     if (!ret) {
         g_warning("Failed to claim accelerometer: %s", error->message);
         g_error_free(error);
         g_clear_object(&iio_proxy);
         g_main_loop_quit(loop);
         return;
     }
     g_variant_unref(ret);
 
     print_initial_values();
 }
 
static void proxy_disconnected (GDBusConnection *connection,
              const gchar *name,
              gpointer user_data)
 {
     if (iio_proxy) {
         g_signal_handlers_disconnect_by_data(iio_proxy, NULL);
         g_clear_object(&iio_proxy);
         g_print("Sensor proxy disconnected\n");
     }
 }
 
void rotation_watch() {
    watch_id = g_bus_watch_name(G_BUS_TYPE_SYSTEM,
                            "net.hadess.SensorProxy",
                            G_BUS_NAME_WATCHER_FLAGS_NONE,
                            proxy_connected,
                            proxy_disconnected,
                            NULL, NULL);

    g_print("Waiting for sensor proxy...\n");
}

void rotation_cleanup() {
    g_bus_unwatch_name(watch_id);

}

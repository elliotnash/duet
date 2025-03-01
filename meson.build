project('duet', 'c', license: 'MIT')

libcjson_proj = subproject('libcjson')
libcjson_dep = libcjson_proj.get_variable('cjson_dep')

libusb_dep = dependency('libusb-1.0')
dbus_dep = dependency('dbus-1')
glib_dep = dependency('glib-2.0')
gio_dep = dependency('gio-2.0')

src_files = [
  'src/main.c',
  'src/display.h',
  'src/display.c',
  'src/keyboard.c',
  'src/keyboard.h',
  'src/rotation.c',
  'src/rotation.h'
]

executable('duet', src_files, dependencies: [libusb_dep, libcjson_dep, dbus_dep, glib_dep, gio_dep])

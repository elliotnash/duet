project('duet', 'c', license: 'MIT')

glib_dep = dependency('glib-2.0')
gio_dep = dependency('gio-2.0')
udev_dep = dependency('libudev')

src_files = [
  'src/main.c',
  'src/display.h',
  'src/display.c',
  'src/keyboard.c',
  'src/keyboard.h',
  'src/rotation.c',
  'src/rotation.h',
  'src/context.c',
  'src/context.h'
]

executable('duet', src_files, install: true, dependencies: [
  glib_dep,
  gio_dep,
  udev_dep
])

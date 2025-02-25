project('duet', 'c')

libcjson_proj = subproject('libcjson')
libcjson_dep = libcjson_proj.get_variable('cjson_dep')

libusb_dep = dependency('libusb-1.0')

src_files = [
  'src/main.c',
  'src/display.h',
  'src/display.c'
]

executable('duet', src_files, dependencies: [libusb_dep, libcjson_dep])

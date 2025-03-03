project('duet', 'c', license: 'MIT', meson_version: '>=1.2.0')

git_branch = run_command(['git', 'rev-parse', '--abbrev-ref', 'HEAD'], capture: true, check: true).stdout().splitlines().get(0)
git_commit = run_command(['git', 'rev-parse', '--short', 'HEAD'], capture: true, check: true).stdout().splitlines().get(0)

git_data = configuration_data()
git_data.set_quoted('GIT_BRANCH', git_branch)
git_data.set_quoted('GIT_COMMIT', git_commit)

git_version_h = configure_file(output : 'git_version.h',
  configuration : git_data,
)

glib_dep = dependency('glib-2.0')
gio_dep = dependency('gio-2.0')
udev_dep = dependency('libudev')

dependencies = [
  glib_dep,
  gio_dep,
  udev_dep
]

src_files = [
  git_version_h,
  'src/display.h',
  'src/display.c',
  'src/keyboard.c',
  'src/keyboard.h',
  'src/rotation.c',
  'src/rotation.h',
  'src/context.c',
  'src/context.h',
  'src/command.c',
  'src/command.h'
]

daemon_src = src_files + ['src/daemon.c']
cli_src = src_files + ['src/cli.c']

executable('duetd', daemon_src, install: true, dependencies: dependencies)
executable('duet', cli_src, install: true, dependencies: dependencies)

# Duet

A utility for managing dual-screen setups on the Asus Zenbook Duo (UX8406) under Hyprland. Automatically adjusts display configuration based on keyboard attachment status and device orientation.

## Features

- 🔄 Automatic secondary display toggling when keyboard is connected/disconnected
- 🧭 Rotation detection for proper screen orientation

## Installation

### Build Requirements
- Meson build system
- Ninja
- GLib2

### Other Requirements

- Hyprland
- iio-sensor-proxy

<details> <summary>Install dependancies on Arch Linux</summary>

```bash
sudo pacman -S meson ninja glib2-devel iio-sensor-proxy
```

</details>

### Build & Install
```bash
git clone https://github.com/yourusername/duet.git
cd duet
meson setup builddir
cd builddir
ninja
sudo meson install
```

## Configuration

Add to your Hyprland config (`~/.config/hypr/hyprland.conf`):

```ini
exec-once = duet
```

## Dependencies

- libglib2.0-dev

- libgio2.0-dev

- libudev-1.0

- iio-sensor-proxy

## Contributing

PRs welcome! Please open an issue first to discuss proposed changes.

## License

MIT License - See LICENSE for details

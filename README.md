# iGame Control for Linux

A Linux replacement for Colorful's iGame Center: RGB lighting with 32 effects (including LedFx-style
music effects and Smart Sound), power limit, overclocking, clock locking, per-fan curves, profiles,
LedFx integration and live monitoring for **Colorful iGame GeForce** graphics cards.

Written in C++ and Qt 6 (QML), so it stays light: about 0.7% of one CPU core in the tray while a
music effect is running.

[![CI](https://github.com/zazazaxky-m/igame-linux/actions/workflows/ci.yml/badge.svg)](https://github.com/zazazaxky-m/igame-linux/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/zazazaxky-m/igame-linux)](https://github.com/zazazaxky-m/igame-linux/releases)
[![License: GPL-3.0-or-later](https://img.shields.io/badge/license-GPL--3.0--or--later-blue)](LICENSE)

[Bahasa Indonesia](README.id.md) · [Changelog](CHANGELOG.md) · [Contributing](CONTRIBUTING.md) · [Security](SECURITY.md)

> **Unofficial project.** Not affiliated with or endorsed by Colorful or NVIDIA. Colorful, iGame and
> iGame Center are trademarks of Colorful Technology; NVIDIA and GeForce are trademarks of NVIDIA
> Corporation. Overclocking and fan control are at your own risk.

| Overview | RGB lighting |
|---|---|
| ![Overview](docs/screenshots/overview.png) | ![RGB lighting](docs/screenshots/lighting.png) |
| **Performance** | **Info & settings** |
| ![Performance](docs/screenshots/performance.png) | ![Info and settings](docs/screenshots/settings.png) |

## Features

**Overview**
- Gauges for temperature, load, power and fans; core, memory and video clocks with their offsets.
- VRAM, encoder/decoder load and the reasons the GPU is not at its maximum clock.
- Two-minute history graphs, and a drawing of the card that glows in the real LED color with fans
  spinning at the real RPM.

**Performance**
- Power limit, core and memory clock offsets (sliders limited to ±500 / −1000..+3000 MHz unless
  "Full range" is on) and a core clock lock. A positive core offset with a clock lock is the way to
  undervolt on Linux.
- Presets: Power saving, Factory default, Maximum power.
- Optional "apply at service start", with crash protection: if the system does not stay up for two
  minutes after the settings were applied at boot, the option turns itself off on the next start.

**Fans**
- Each fan on its own or all together; Automatic (driver), Fixed % or Curve.
- Curve editor (drag points, double-click to add, right-click to remove) with Silent, Balanced,
  Aggressive and Maximum presets, and 0 RPM below a chosen temperature.
- Fail-safe: if the temperature cannot be read, or the service stops, fans go back to the driver.

**RGB lighting**

| Group | Effects |
|---|---|
| Static and animated | Static, Breathing, Rainbow breathing, Spectrum, Color cycle, Aurora, Plasma, Ocean, Heartbeat, Candle, Twinkle, Lightning, Glitch, Flash, Double flash, Strobe, Police, Time of day, Off |
| Sensors | Follow temperature, Follow load, Follow power |
| Audio (LedFx style) | Smart Sound, Bar, Energy, Scroll, Power, BPM Strobe, Real Strobe, Wavelength, Magnitude, Ambient |

- Palettes of up to 8 colors with an HSV color picker, remembered per effect; speed and brightness.
- Animated effects run in the service, so they keep going when the app is closed.

**Audio effects**
- Desktop audio is captured with `parec` (the default output's monitor), so the app has to keep
  running, at least in the tray.
- Audio is processed the way [LedFx](https://www.ledfx.app) does it: a 24-band melbank from a
  4096-point FFT 60 times a second, peak isolation, automatic gain and LedFx's smoothing filters.
  The effects follow LedFx's Bar, Energy, Scroll, Power, BPM Strobe, Real Strobe, Wavelength and
  Magnitude, adapted to a single LED (roughly what a whole LedFx strip looks like from a distance)
  and gamma corrected like WLED.
- The palette works as the LedFx gradient; Energy and Scroll take three colors for bass, mids and
  treble.
- Tempo and beats (for Bar and BPM Strobe) come from a beat tracker that locks onto the kick drum,
  so off-beat bass notes do not pull it off the beat.
- **Sound sensitivity** 10–300%.
- **Smart Sound** recognises the style of the music from its bass, brightness, onset density and
  tempo, and picks a LedFx effect and colors for it: Calm (Wavelength), Pop (Energy), Electronic
  (Power), Hip-hop (Bar), Rock/metal (Magnitude), Voice (warm glow) or Silence. Styles change with a
  1.5 s crossfade and only after several seconds, so it does not flip back and forth. Detection is
  heuristic; mixed styles end up in the closest match.

**LedFx**
- The service accepts a [LedFx](https://www.ledfx.app) stream over DDP (UDP port 4048). While LedFx
  is sending it controls the light; the effect chosen in the app comes back 2 seconds after LedFx
  stops.
- In LedFx: **Devices → Add Device → DDP**, IP address `127.0.0.1`, port `4048`, pixel count `1`,
  then assign an effect. 30 fps is enough, the card shows at most 30 frames per second. With more
  than one pixel the colors are averaged.
- Only LedFx on the same computer can connect by default. For LedFx on another machine, enable
  **Accept LedFx from other devices** in Info & settings and use this computer's IP address.

**Other**
- **Profiles:** save a combination of performance, fan and lighting settings and apply it with one
  click.
- **App:** tray icon, start at login (hidden in the tray), a single instance, and
  `igame-control --toggle` to show or hide the window from a keyboard shortcut.
- **Languages:** English and Indonesian, switchable live in **Info & settings → Application →
  Language**.

## Supported hardware

| | Status |
|---|---|
| Colorful iGame GeForce RTX 5070 Ultra W OC 12GB-V (PCI subsystem `7377:1501`, controller `0x5501`, type `0x17`, 1 LED) | Tested: everything works |
| Other Colorful iGame cards (subsystem vendor `7377`) | The controller is detected, but colors are only sent to type `0x17`. Reports welcome. |
| Any NVIDIA GPU supported by NVML | Performance, fans and monitoring should work; lighting does not apply |

Voltage control is not available on Linux.

## Install

You need the proprietary NVIDIA driver. The installers below take care of everything else:
building, the `igamed` service and the `i2c-dev` module. Afterwards open **iGame Control** from
the application menu. The app talks to the service as a member of the administrators' group
(`wheel`, or `sudo` on Debian and Ubuntu), which your user normally already is.

### Arch Linux and derivatives

Download `igame-linux-*.pkg.tar.zst` from the [latest release](https://github.com/zazazaxky-m/igame-linux/releases/latest),
then in the folder you saved it to:

```bash
sudo pacman -U igame-linux-*.pkg.tar.zst
```

```bash
sudo systemctl enable --now igamed
```

pacman manages it from then on; install a newer release the same way. The install script below
also works on Arch and builds the same package from source.

### Any distribution: the install script

```bash
git clone https://github.com/zazazaxky-m/igame-linux.git && cd igame-linux && ./install.sh
```

The script installs the build dependencies (pacman, apt, dnf or zypper), builds, installs and
starts the service. On Arch it builds a real package, so pacman manages it. To remove it again:

```bash
./install.sh --uninstall
```

Only Arch Linux is tested so far. Debian and Ubuntu need Qt 6.5 or newer (Debian 13, Ubuntu
25.04 or newer). Reports from other distributions are welcome.

### Manually with CMake

Requirements: a C++20 compiler, CMake 3.21+, Qt 6.5+ (Core, Gui, Network, Quick, Quick Controls 2,
Widgets, Svg, Linguist tools; QML modules QtQuick.Controls, Layouts, Shapes, Effects and QtCore at
runtime) and `parec` (PulseAudio or PipeWire-Pulse) for the audio effects.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
```

```bash
cmake --build build -j"$(nproc)" && ctest --test-dir build
```

```bash
sudo cmake --install build && sudo systemctl daemon-reload && sudo systemctl enable --now igamed
```

## Command line

`igame-ctl --help` lists everything. Examples:

```bash
igame-ctl status
igame-ctl rgb cycle -c FF0040,7A00FF,00C8FF -s 1.5 -b 80
igame-ctl rgb temperature --low 40 --high 80
igame-ctl rgb music_smart --sens 140
igame-ctl ledfx on --network off
igame-ctl oc --power 260 --core 150 --mem 1000
igame-ctl oc --lock 210:2600
igame-ctl fan curve 40:30,60:50,75:75,85:100 --zero-rpm 45
igame-ctl fan 60 --fan 2
igame-ctl fan auto
igame-ctl profile save Gaming
igame-ctl profile load Gaming
igame-ctl boot on
igame-ctl reset
```

## Resource usage

Measured on a Ryzen 7 9700X with an RTX 5070. CPU is a percentage of one core.

| Situation | CPU | RAM (PSS) |
|---|---|---|
| `igamed`, static light | ~0.4% | ~35 MB |
| `igamed`, animated or music effect | ~5–7% | ~35 MB |
| `igamed`, driven by LedFx (60 fps in, 30 to the card) | ~7% | ~35 MB |
| App in the tray, music effect running (including `parec`) | ~0.7–1% | ~23 MB |
| App in the tray while LedFx drives the light | ~0.3% | ~23 MB |
| Window open on the RGB page | ~8% | ~120–190 MB |

- Almost all of the service's load is the NVIDIA driver: every color sent over I2C costs about
  2.3 ms of CPU, at most 30 times a second. Unchanged colors are not sent again.
- The audio analysis itself (both FFTs, melbank, tempo and beat tracking) takes about 0.5%.
- Closing the window to the tray restarts the app as a small tray process, so the window's memory
  is really released.

## How it works

| Program | Role |
|---|---|
| `igamed` | Root service (systemd). Owns NVML and I2C, runs the lighting effects, fan curves, profiles and the LedFx receiver. Keeps running when the app is closed. |
| `igame-control` | The app. Runs as your user, talks to `igamed` over a Unix socket (newline-delimited JSON), and computes the audio effects. |
| `igame-ctl` | Command line client for terminals and scripts. |

**Lighting.** The controller sits on the GPU's I2C bus (`NVIDIA i2c adapter 1`) at address `0x61`.
The protocol was worked out from iGame Center 2.0.0.82 for interoperability, and matches the
ColorfulGPU controller in [OpenRGB](https://openrgb.org):

- Packet: `AA EF <cmd lo> <cmd hi> <data> <16-bit sum of all previous bytes>`.
- Handshake `0x0281`: write, wait 50 ms, read 64 bytes; bytes 4–5 of the reply are the model ID
  (`0x5501` = type `0x17`, 1 LED). Retried up to five times.
- Color frame `0x0312`: `01 FF R G B`.
- Effects are computed on the PC and streamed at about 30 frames per second, like iGame Center
  does. An unchanged color is resent every 3 seconds so the light recovers after suspend.

**Power, overclocking and fans** use NVML (`libnvidia-ml.so.1`), the Linux counterpart of the
NVAPI calls iGame Center makes on Windows: `nvmlDeviceSetPowerManagementLimit`,
`nvmlDeviceSetClockOffsets`, `nvmlDeviceSetGpuLockedClocks` and `nvmlDeviceSetFanSpeed_v2` per fan.

**Files**
- Configuration and profiles: `/etc/igame-linux/config.json`
- Socket: `/run/igame-linux/igamed.sock`
- Logs: `journalctl -u igamed`

## Development

See [CONTRIBUTING.md](CONTRIBUTING.md) for building, running a test copy of the service without
root, the code layout, tests and translations.

## Credits

- [LedFx](https://github.com/LedFx/LedFx): the audio processing and the music effects are
  reimplemented from its design.
- [OpenRGB](https://gitlab.com/CalcProgrammer1/OpenRGB): its ColorfulGPU controller was used to
  cross-check the lighting protocol.

## License

GNU General Public License v3.0 or later. See [LICENSE](LICENSE).

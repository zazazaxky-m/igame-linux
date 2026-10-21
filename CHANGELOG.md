# Changelog

All notable changes to this project are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project uses
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.0.0] - 2026-10-21

First public release.

### Added

- `igamed` root service: Colorful iGame lighting over the GPU's I2C bus, NVML power limit, clock
  offsets, clock lock and per-fan control, profiles, and a Unix socket protocol for clients.
- `igame-control` app (Qt 6 / QML) with Overview, Performance, Fans, RGB lighting, Profiles and
  Info & settings pages, a tray icon, start at login and `--toggle`.
- `igame-ctl` command line client.
- 32 lighting effects: static and animated, sensor-driven, and audio effects modelled on LedFx
  (Bar, Energy, Scroll, Power, BPM Strobe, Real Strobe, Wavelength, Magnitude, Ambient), plus
  Smart Sound, which recognises the style of music and picks an effect for it.
- LedFx integration: `igamed` accepts DDP streams on UDP port 4048 and hands the light back to the
  chosen effect when LedFx stops.
- Optional apply-at-boot for performance settings, with crash protection.
- English and Indonesian translations.
- Automated tests (Qt Test / CTest) for the lighting protocol, effect settings, fan curves, the
  DDP receiver and the audio engine.

[Unreleased]: https://github.com/zazazaxky-m/igame-linux/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/zazazaxky-m/igame-linux/releases/tag/v1.0.0

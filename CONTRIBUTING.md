# Contributing

Thanks for helping! Bug reports, reports from other Colorful cards, translations and code are
all welcome. Please follow the [Code of Conduct](CODE_OF_CONDUCT.md).

## Reporting

- **Bugs:** open an issue with the "Bug report" form. Include `igame-ctl status` and
  `journalctl -u igamed -b --no-pager | tail -50`.
- **Another Colorful card:** use the "Report another card" form. The lighting protocol is only
  enabled for controller type `0x17`; reports from other cards are how support grows.
- **Security problems:** do not open an issue, see [SECURITY.md](SECURITY.md).

## Building

You need a C++20 compiler, CMake 3.21+ and Qt 6.5+ (Core, Gui, Network, Quick, Quick Controls 2,
Widgets, Svg, Test and the Linguist tools).

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
```

```bash
cmake --build build -j"$(nproc)"
```

```bash
ctest --test-dir build --output-on-failure
```

To try changes without touching the installed service, run a second `igamed` as your user on its
own socket (power, clocks and fans are then refused by the driver, lighting needs access to
`/dev/i2c-*`):

```bash
./build/daemon/igamed --socket /run/user/$UID/igame-test.sock --config /tmp/igame-test.json --state /tmp/igame-state
```

```bash
IGAME_SOCKET=/run/user/$UID/igame-test.sock ./build/gui/igame-control
```

## Layout

| Folder | What is in it |
|---|---|
| `daemon/` | `igamed`: socket protocol (`daemon.cpp`), lighting effects (`effects.cpp`), I2C controller (`rgbcontroller.cpp`), NVML wrapper, fan curves, LedFx/DDP receiver |
| `gui/` | `igame-control`: QML pages and components, socket client, audio capture and the audio engine (`audioanalyzer.cpp`) |
| `cli/` | `igame-ctl` |
| `common/` | Socket protocol helpers and error texts shared by all three |
| `dist/` | systemd unit, desktop entry, AppStream metadata, modules-load file |
| `tests/` | Qt Test unit tests, run by CTest |
| `packaging/` | Distribution packaging (AUR) |

## Code style

- Follow the surrounding code. `.clang-format` and `.editorconfig` describe the style (4 spaces,
  braces on their own line for functions and classes). Format the lines you change, not whole
  files, so diffs stay readable.
- Comments explain why something is done, in plain English.
- Keep the app light: it runs all day in the tray. Measure CPU use for anything that runs per
  frame or per audio block.
- New behaviour gets a test in `tests/` when it can be tested without hardware.

## Translations

User-visible text is written in English with `qsTr()` (QML) or `tr()` (C++). Indonesian lives in
`gui/i18n/igame_id.ts` and `cli/i18n/igame-ctl_id.ts`.

To refresh the `.ts` files after changing text, then translate with Qt Linguist:

```bash
cmake --build build --target update_translations
```

```bash
linguist6 gui/i18n/igame_id.ts
```

To add a language, add a `.ts` file next to the existing one and list it in `TS_FILES` in
`gui/CMakeLists.txt` and `cli/CMakeLists.txt`.

## Pull requests

- Keep each pull request to one topic and describe how you tested it (which card, which effect).
- Make sure `ctest` passes; CI builds and tests every pull request.
- Add a line under `[Unreleased]` in [CHANGELOG.md](CHANGELOG.md) for user-visible changes.
- Contributions are licensed under the project's license, GPL-3.0-or-later.

## Releases

1. Move the `[Unreleased]` entries in `CHANGELOG.md` under a new version heading.
2. Update the version in `CMakeLists.txt`, `PKGBUILD` and the `<releases>` list in
   `dist/io.github.zazazaxky_m.IGameControl.metainfo.xml`.
3. Tag and push (`git tag v1.1.0 && git push --tags`). The release workflow builds the Arch package
   and publishes the GitHub release with the changelog section as notes.

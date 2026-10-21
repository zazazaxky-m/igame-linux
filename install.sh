#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Build + install iGame Control from this checkout and start igamed.
#
#   ./install.sh               install (uses sudo when needed)
#   ./install.sh --uninstall   remove it (/etc/igame-linux is kept)
#
# Arch: builds a pacman package with makepkg. Elsewhere: CMake install to /usr/local.
#
# Options:
#   --prefix DIR   install prefix for the CMake path (default /usr/local)
#   --cmake        use the CMake path even on Arch
#   --no-deps      don't install build dependencies
#   --no-service   don't enable/start igamed
#   --uninstall    remove a previous install
set -euo pipefail

cd "$(dirname "$(readlink -f "$0")")"

prefix=/usr/local
use_cmake=0
install_deps=1
service=1
action=install

while [ $# -gt 0 ]; do
    case "$1" in
    --prefix) prefix="$2"; shift ;;
    --cmake) use_cmake=1 ;;
    --no-deps) install_deps=0 ;;
    --no-service) service=0 ;;
    --uninstall) action=uninstall ;;
    -h | --help) awk 'NR > 2 && /^#/ { sub(/^# ?/, ""); print; next } NR > 2 { exit }' "$0"; exit 0 ;;
    *) echo "Unknown option: $1 (see --help)" >&2; exit 2 ;;
    esac
    shift
done

say() { printf '\033[1;36m==>\033[0m %s\n' "$*"; }
warn() { printf '\033[1;33m==> warning:\033[0m %s\n' "$*" >&2; }
die() { printf '\033[1;31m==> error:\033[0m %s\n' "$*" >&2; exit 1; }

# only use sudo if we can't write to the prefix ourselves
prefix_writable() {
    local dir="$prefix"
    while [ ! -e "$dir" ]; do dir="$(dirname "$dir")"; done
    [ -w "$dir" ]
}

as_root() {
    if [ "$(id -u)" -eq 0 ] || prefix_writable; then
        "$@"
    else
        sudo "$@"
    fi
}

manifest="$prefix/share/igame-linux/install_manifest.txt"

is_arch() { command -v pacman >/dev/null && command -v makepkg >/dev/null; }

check_driver() {
    # no grep -q here, it breaks the pipe under pipefail
    if ! { ldconfig -p 2>/dev/null || /sbin/ldconfig -p 2>/dev/null || true; } | grep 'libnvidia-ml\.so\.1' >/dev/null; then
        warn "The NVIDIA driver (libnvidia-ml.so.1) was not found. Power, overclock, fans and"
        warn "monitoring need the proprietary NVIDIA driver; install it before using the app."
    fi
}

install_dependencies() {
    [ "$install_deps" -eq 1 ] || return 0
    if command -v pacman >/dev/null; then
        say "Installing build dependencies (pacman)"
        sudo pacman -S --needed --noconfirm base-devel cmake qt6-base qt6-declarative qt6-svg qt6-tools libpulse
    elif command -v apt-get >/dev/null; then
        say "Installing build dependencies (apt); needs Qt 6.5 or newer (Debian 13, Ubuntu 25.04 or newer)"
        sudo apt-get update
        sudo apt-get install -y build-essential cmake qt6-base-dev qt6-declarative-dev qt6-tools-dev \
            qt6-tools-dev-tools qt6-l10n-tools libqt6svg6-dev qml6-module-qtquick qml6-module-qtquick-controls \
            qml6-module-qtquick-layouts qml6-module-qtquick-shapes qml6-module-qtquick-effects \
            qml6-module-qtquick-templates qml6-module-qtquick-window qml6-module-qtqml-workerscript \
            qml6-module-qtcore pulseaudio-utils
    elif command -v dnf >/dev/null; then
        say "Installing build dependencies (dnf)"
        sudo dnf install -y gcc-c++ cmake qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qttools-devel \
            qt6-qtsvg-devel pulseaudio-utils
    elif command -v zypper >/dev/null; then
        say "Installing build dependencies (zypper)"
        sudo zypper --non-interactive install gcc-c++ cmake qt6-base-devel qt6-declarative-devel \
            qt6-tools-linguist qt6-svg-devel pulseaudio-utils
    else
        warn "Unknown package manager: install a C++20 compiler, CMake, Qt 6.5+ (Core, Gui, Network,"
        warn "Quick, Quick Controls 2, Widgets, Svg, Linguist tools) and parec yourself, then rerun"
        warn "with --no-deps."
    fi
}

enable_service() {
    [ "$service" -eq 1 ] || return 0
    say "Starting the igamed service"
    sudo systemctl daemon-reload
    sudo systemctl enable --now igamed.service
    sudo systemctl restart igamed.service # pick up a new binary when upgrading
}

check_group() {
    local g
    for g in wheel sudo admin; do
        if id -nG "$USER" | tr ' ' '\n' | grep -x "$g" >/dev/null; then
            return 0
        fi
    done
    warn "Your user is not in the wheel/sudo group, which the app needs to talk to igamed."
    warn "Add yourself (for example: sudo usermod -aG wheel $USER), then log out and in again."
}

# refresh menu/icon caches
refresh_caches() {
    if command -v update-desktop-database >/dev/null; then
        as_root update-desktop-database -q "$prefix/share/applications" || true
    fi
    if command -v gtk-update-icon-cache >/dev/null; then
        as_root gtk-update-icon-cache -qtf "$prefix/share/icons/hicolor" || true
    fi
}

install_arch() {
    say "Building and installing the package with makepkg"
    install_dependencies
    makepkg -sif --noconfirm
    rm -rf build-pkg pkg src
}

install_cmake() {
    install_dependencies
    local build=build-install
    say "Building (this takes a minute)"
    cmake -B "$build" -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$prefix" -DBUILD_TESTING=OFF
    cmake --build "$build" --parallel "$(nproc 2>/dev/null || echo 2)"
    say "Installing to $prefix"
    as_root cmake --install "$build"
    # keep the file list for --uninstall
    as_root install -Dm644 "$build/install_manifest.txt" "$manifest"
    refresh_caches
}

uninstall() {
    if is_arch && [ "$use_cmake" -eq 0 ] && pacman -Q igame-linux >/dev/null 2>&1; then
        say "Removing the igame-linux package"
        sudo pacman -R --noconfirm igame-linux
        return
    fi
    [ -f "$manifest" ] || die "Nothing to remove: $manifest not found (was it installed with --prefix?)"
    if [ "$service" -eq 1 ] && command -v systemctl >/dev/null; then
        sudo systemctl disable --now igamed.service 2>/dev/null || true
    fi
    say "Removing the installed files"
    # cmake doesn't end the file with a newline
    while IFS= read -r file || [ -n "$file" ]; do
        [ -n "$file" ] && as_root rm -f "$file"
    done <"$manifest"
    as_root rm -f "$manifest"
    as_root rmdir --ignore-fail-on-non-empty -p "$(dirname "$manifest")" 2>/dev/null || true
    as_root rmdir --ignore-fail-on-non-empty "$prefix/share/doc/igame-linux" 2>/dev/null || true
    refresh_caches
    if [ "$service" -eq 1 ] && command -v systemctl >/dev/null; then
        sudo systemctl daemon-reload
    fi
    say "Done. Settings in /etc/igame-linux were kept; delete that folder to remove them too."
}

if [ "$action" = uninstall ]; then
    uninstall
    exit 0
fi

check_driver
if is_arch && [ "$use_cmake" -eq 0 ]; then
    install_arch
else
    install_cmake
fi
enable_service
check_group
say "Done. Open \"iGame Control\" from your application menu, or run igame-ctl status."

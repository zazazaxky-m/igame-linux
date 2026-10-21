# Maintainer: zazazaxky-m
# Builds a package from this checkout: run `makepkg -si` in the repository root.
# For the AUR package that builds from GitHub, see packaging/aur/.
pkgname=igame-linux
pkgver=1.0.0
pkgrel=1
pkgdesc="Unofficial lighting, overclocking and fan control for Colorful iGame GeForce cards (iGame Center replacement)"
arch=('x86_64')
url="https://github.com/zazazaxky-m/igame-linux"
license=('GPL-3.0-or-later')
depends=('qt6-base' 'qt6-declarative' 'qt6-svg' 'nvidia-utils' 'libpulse' 'hicolor-icon-theme')
makedepends=('cmake' 'qt6-tools')
install=igame-linux.install

build() {
    cmake -B "$startdir/build-pkg" -S "$startdir" \
        -DCMAKE_BUILD_TYPE=None \
        -DCMAKE_INSTALL_PREFIX=/usr
    cmake --build "$startdir/build-pkg"
}

check() {
    ctest --test-dir "$startdir/build-pkg" --output-on-failure
}

package() {
    DESTDIR="$pkgdir" cmake --install "$startdir/build-pkg"
    install -Dm644 "$startdir/LICENSE" "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}

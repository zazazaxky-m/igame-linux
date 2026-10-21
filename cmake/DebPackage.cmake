# SPDX-License-Identifier: GPL-3.0-or-later
# cpack -G DEB, configure with -DCMAKE_INSTALL_PREFIX=/usr. Built on Debian 13 by the release
# workflow; the same package installs on Ubuntu 26.04.
set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_NAME igame-linux)
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_CONTACT "zazazaxky-m <78693013+zazazaxky-m@users.noreply.github.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Lighting, overclocking and fan control for Colorful iGame GeForce cards")
set(CPACK_PACKAGE_DESCRIPTION "Unofficial replacement for Colorful's iGame Center. RGB lighting with music effects
modelled on LedFx, power limit, clock offsets, fan curves, profiles and a LedFx DDP receiver.
Needs the proprietary NVIDIA driver.")
set(CPACK_PACKAGE_HOMEPAGE_URL ${PROJECT_HOMEPAGE_URL})
set(CPACK_PACKAGING_INSTALL_PREFIX /usr)
set(CPACK_STRIP_FILES ON)

set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_RELEASE 1)
set(CPACK_DEBIAN_PACKAGE_SECTION utils)
find_program(DPKG dpkg)
if(NOT DPKG)
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE amd64) # only matters when packaging off Debian
endif()
find_program(DPKG_SHLIBDEPS dpkg-shlibdeps)
if(DPKG_SHLIBDEPS)
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON) # library dependencies are worked out automatically
endif()
# QML modules and runtime tools aren't linked, so list them by hand
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "qml6-module-qtquick, qml6-module-qtquick-controls, qml6-module-qtquick-templates, qml6-module-qtquick-layouts, qml6-module-qtquick-shapes, qml6-module-qtquick-effects, qml6-module-qtquick-window, qml6-module-qtqml-workerscript, qml6-module-qtcore, qt6-svg-plugins, qt6-qpa-plugins, pulseaudio-utils, kmod")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "qt6-wayland")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    ${PROJECT_SOURCE_DIR}/dist/debian/postinst
    ${PROJECT_SOURCE_DIR}/dist/debian/prerm
    ${PROJECT_SOURCE_DIR}/dist/debian/postrm)
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION ON)

include(CPack)

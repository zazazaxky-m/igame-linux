# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.21)

project(igame-linux
    VERSION 1.0.0
    DESCRIPTION "Lighting, overclocking and fan control for Colorful iGame GeForce cards"
    HOMEPAGE_URL "https://github.com/zazazaxky-m/igame-linux"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(GNUInstallDirs)
include(CTest) # BUILD_TESTING, on by default

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Network Quick QuickControls2 Widgets Svg LinguistTools)
if(BUILD_TESTING)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Test)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra)
endif()
add_compile_definitions(IGAME_VERSION="${PROJECT_VERSION}")

# Relative to the install prefix. systemd also reads units and modules-load.d from
# /usr/local/lib, so the defaults work for both /usr and /usr/local installs.
set(IGAME_SYSTEMD_UNITDIR "lib/systemd/system" CACHE STRING "Directory for igamed.service")
set(IGAME_MODULES_LOAD_DIR "lib/modules-load.d" CACHE STRING "Directory for the i2c-dev modules-load file")

add_subdirectory(daemon)
add_subdirectory(gui)
add_subdirectory(cli)
add_subdirectory(dist)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

install(FILES README.md README.id.md CHANGELOG.md LICENSE DESTINATION ${CMAKE_INSTALL_DOCDIR})

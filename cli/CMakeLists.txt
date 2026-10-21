# SPDX-License-Identifier: GPL-3.0-or-later
qt_add_executable(igame-ctl main.cpp)
target_include_directories(igame-ctl PRIVATE ${PROJECT_SOURCE_DIR}/common)
target_link_libraries(igame-ctl PRIVATE Qt6::Core Qt6::Network)

# embedded as :/i18n/igame-ctl_id.qm
qt_add_translations(igame-ctl TS_FILES i18n/igame-ctl_id.ts RESOURCE_PREFIX /i18n)

install(TARGETS igame-ctl RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

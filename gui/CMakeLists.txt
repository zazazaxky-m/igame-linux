# SPDX-License-Identifier: GPL-3.0-or-later
# separate lib so it can be tested without the UI
add_library(igame_audio STATIC audioanalyzer.cpp audioanalyzer.h realfft.h)
target_include_directories(igame_audio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(igame_audio PUBLIC Qt6::Gui)

qt_add_executable(igame-control
    apphelper.cpp apphelper.h
    audiocapture.cpp audiocapture.h
    daemonclient.cpp daemonclient.h
    main.cpp
    qml.qrc)
target_include_directories(igame-control PRIVATE ${PROJECT_SOURCE_DIR}/common)
target_link_libraries(igame-control PRIVATE igame_audio Qt6::Quick Qt6::QuickControls2 Qt6::Widgets Qt6::Svg Qt6::Network)

# embedded as :/i18n/igame_id.qm
qt_add_translations(igame-control TS_FILES i18n/igame_id.ts RESOURCE_PREFIX /i18n)

install(TARGETS igame-control RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES icons/app.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps RENAME igame-control.svg)

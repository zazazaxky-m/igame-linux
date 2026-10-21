# SPDX-License-Identifier: GPL-3.0-or-later
# static lib so the tests can link it
add_library(igamed_core STATIC
    daemon.cpp daemon.h
    ddpreceiver.cpp ddpreceiver.h
    effects.cpp effects.h
    fancontroller.cpp fancontroller.h
    nvml.cpp nvml.h
    rgbcontroller.cpp rgbcontroller.h
    ${PROJECT_SOURCE_DIR}/common/protocol.h)
target_include_directories(igamed_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/common)
target_link_libraries(igamed_core PUBLIC Qt6::Core Qt6::Gui Qt6::Network ${CMAKE_DL_LIBS})

qt_add_executable(igamed main.cpp)
target_link_libraries(igamed PRIVATE igamed_core)

install(TARGETS igamed RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

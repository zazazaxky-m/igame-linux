# SPDX-License-Identifier: GPL-3.0-or-later
function(igame_add_test name)
    qt_add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE Qt6::Test ${ARGN})
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 60)
endfunction()

igame_add_test(test_rgbprotocol igamed_core)
igame_add_test(test_effects igamed_core)
igame_add_test(test_fans igamed_core)
igame_add_test(test_ddp igamed_core)
igame_add_test(test_audioanalyzer igame_audio)

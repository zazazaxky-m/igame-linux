# SPDX-License-Identifier: GPL-3.0-or-later
configure_file(igamed.service.in igamed.service @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/igamed.service DESTINATION ${IGAME_SYSTEMD_UNITDIR})
install(FILES igame-linux-modules.conf DESTINATION ${IGAME_MODULES_LOAD_DIR} RENAME igame-linux.conf)
install(FILES igame-control.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES io.github.zazazaxky_m.IGameControl.metainfo.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)

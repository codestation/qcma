add_executable(qcma
    main.cpp mainwidget.cpp singleapplication.cpp clientmanager.cpp filterlineedit.cpp qtrayicon.cpp
    trayindicator.h trayindicator_global.h
    forms/backupitem.cpp forms/backupmanagerform.cpp forms/configwidget.cpp
    forms/confirmdialog.cpp forms/pinform.cpp forms/progressform.cpp
    forms/configwidget.ui forms/backupmanagerform.ui forms/backupitem.ui
    forms/confirmdialog.ui forms/progressform.ui forms/pinform.ui
    gui.qrc)
target_include_directories(qcma PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

if(WIN32)
    target_sources(qcma PRIVATE qcma.rc)
endif()
if(APPLE)
    target_sources(qcma PRIVATE resources/images/qcma.icns)
    set_source_files_properties(resources/images/qcma.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_target_properties(qcma PROPERTIES MACOSX_BUNDLE TRUE MACOSX_BUNDLE_ICON_FILE qcma.icns)
endif()
if(UNIX AND NOT APPLE)
    pkg_check_modules(LIBNOTIFY REQUIRED IMPORTED_TARGET libnotify)
    target_link_libraries(qcma PRIVATE PkgConfig::LIBNOTIFY)
endif()

qt_add_translations(qcma TS_FILES ${QCMA_TS_FILES}
    RESOURCE_PREFIX /resources/translations)
target_link_libraries(qcma PRIVATE qcma_common Qt6::Gui Qt6::Widgets)

install(TARGETS qcma BUNDLE DESTINATION . RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
if(UNIX AND NOT APPLE)
    install(FILES resources/qcma.desktop DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
    install(FILES resources/images/qcma.png DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/64x64/apps")
    install(FILES qcma.1 DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
endif()

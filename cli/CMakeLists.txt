add_executable(qcma_cli main_cli.cpp singlecoreapplication.cpp headlessmanager.cpp)
qt_add_translations(qcma_cli TS_FILES ${QCMA_TS_FILES}
    RESOURCE_PREFIX /resources/translations)
target_link_libraries(qcma_cli PRIVATE qcma_common)

install(TARGETS qcma_cli RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES qcma_cli.1 DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")

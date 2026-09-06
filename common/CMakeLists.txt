if(NOT WIN32)
    pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavformat libavcodec libavutil libswscale)
endif()

add_library(qcma_common STATIC
    capability.cpp cmaobject.cpp cmarootobject.cpp cmautils.cpp sforeader.cpp
    cmaclient.cpp cmabroadcast.cpp cmaevent.cpp dds.cpp sqlitedb.cpp
    httpdownloader.cpp qlistdb.cpp database.cpp avdecoder.cpp
    commonrc.qrc)
target_include_directories(qcma_common PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(qcma_common PUBLIC FFMPEG_ENABLED)
target_link_libraries(qcma_common PUBLIC Qt6::Core Qt6::Gui Qt6::Network Qt6::Sql VitaMTP::VitaMTP)
if(WIN32)
    target_include_directories(qcma_common PRIVATE ${FFMPEG_INCLUDE_DIRS})
    target_link_libraries(qcma_common PUBLIC ${FFMPEG_LIBRARIES})
else()
    target_link_libraries(qcma_common PUBLIC PkgConfig::FFMPEG)
endif()

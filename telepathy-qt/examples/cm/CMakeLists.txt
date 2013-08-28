if(ENABLE_EXPERIMENTAL_SERVICE_SUPPORT)
    set(cm_SRCS
        protocol.h
        protocol.cpp
        main.cpp)

    set(cm_MOC_SRCS
        protocol.h)

    tpqt_generate_mocs(${cm_MOC_SRCS})

    add_executable(cm ${cm_SRCS} ${cm_MOC_SRCS})
    target_link_libraries(cm
        ${Qt5Core_LIBRARIES}
        ${Qt5DBus_LIBRARIES}
        ${Qt5Network_LIBRARIES}
        ${Qt5Xml_LIBRARIES}
        telepathy-qt${Qt5Core_VERSION_MAJOR}
        telepathy-qt${Qt5Core_VERSION_MAJOR}-service
        ${TP_QT_EXECUTABLE_LINKER_FLAGS})
endif(ENABLE_EXPERIMENTAL_SERVICE_SUPPORT)

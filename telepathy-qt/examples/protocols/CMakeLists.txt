set(protocols_SRCS
    main.cpp
    cm-wrapper.cpp
    protocols.cpp)

set(protocols_MOC_SRCS
    cm-wrapper.h
    protocols.h)

tpqt_generate_mocs(${protocols_MOC_SRCS})

add_executable(protocols ${protocols_SRCS} ${protocols_MOC_SRCS})
target_link_libraries(protocols
    ${Qt5Core_LIBRARIES}
    ${Qt5DBus_LIBRARIES}
    ${Qt5Gui_LIBRARIES}
    ${Qt5Network_LIBRARIES}
    ${Qt5Widgets_LIBRARIES}
    ${Qt5Xml_LIBRARIES}
    telepathy-qt${Qt5Core_VERSION_MAJOR}
    ${TP_QT_EXECUTABLE_LINKER_FLAGS})

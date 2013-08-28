set(tubereceiver_SRCS
    tube-receiver.cpp)

set(tubereceiver_MOC_SRCS
    tube-receiver.h)

tpqt_generate_mocs(${tubereceiver_MOC_SRCS})

add_executable(tubereceiver ${tubereceiver_SRCS} ${tubereceiver_MOC_SRCS})
target_link_libraries(tubereceiver
    ${Qt5Core_LIBRARIES}
    ${Qt5DBus_LIBRARIES}
    ${Qt5Network_LIBRARIES}
    ${Qt5Xml_LIBRARIES}
    telepathy-qt${Qt5Core_VERSION_MAJOR}
    ${TP_QT_EXECUTABLE_LINKER_FLAGS})

set(tubeinitiator_SRCS
    tube-initiator.cpp)

set(tubeinitiator_MOC_SRCS
    tube-initiator.h)

tpqt_generate_mocs(${tubeinitiator_MOC_SRCS})

add_executable(tubeinitiator ${tubeinitiator_SRCS} ${tubeinitiator_MOC_SRCS})
target_link_libraries(tubeinitiator
    ${Qt5DBus_LIBRARIES}
    ${Qt5Gui_LIBRARIES}
    ${Qt5Network_LIBRARIES}
    ${Qt5Xml_LIBRARIES}
    telepathy-qt${Qt5Core_VERSION_MAJOR}
    ${TP_QT_EXECUTABLE_LINKER_FLAGS})

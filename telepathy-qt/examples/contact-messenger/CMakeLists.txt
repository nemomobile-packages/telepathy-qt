set(messenger-sender_SRCS
    sender.cpp)

set(messenger-sender_MOC_SRCS
    sender.h)

tpqt_generate_mocs(${messenger-sender_MOC_SRCS})

add_executable(messenger-sender ${messenger-sender_SRCS} ${messenger-sender_MOC_SRCS})
target_link_libraries(messenger-sender
    ${Qt5Core_LIBRARIES}
    ${Qt5DBus_LIBRARIES}
    ${Qt5Network_LIBRARIES}
    ${Qt5Xml_LIBRARIES}
    telepathy-qt${Qt5Core_VERSION_MAJOR}
    ${TP_QT_EXECUTABLE_LINKER_FLAGS})

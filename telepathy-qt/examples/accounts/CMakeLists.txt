set(accounts_SRCS
    main.cpp
    account-item.cpp
    account-item.h
    accounts-window.cpp
    accounts-window.h)

set(accounts_MOC_SRCS
    account-item.h
    accounts-window.h)

tpqt_generate_mocs(${accounts_MOC_SRCS})

add_executable(accounts ${accounts_SRCS} ${accounts_MOC_SRCS})
target_link_libraries(accounts
    ${Qt5Core_LIBRARIES}
    ${Qt5DBus_LIBRARIES}
    ${Qt5Gui_LIBRARIES}
    ${Qt5Network_LIBRARIES}
    ${Qt5Widgets_LIBRARIES}
    ${Qt5Xml_LIBRARIES}
    telepathy-qt${Qt5Core_VERSION_MAJOR}
    ${TP_QT_EXECUTABLE_LINKER_FLAGS})

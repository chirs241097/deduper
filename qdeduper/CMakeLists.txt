set(CMAKE_INCLUDE_CURRENT_DIR ON)

option(QDEDUPER_USE_QT6 "Build qdeduper with Qt 6" OFF)

if (QDEDUPER_USE_QT6)
    find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent OPTIONAL_COMPONENTS DBus)
else()
    find_package(Qt5 REQUIRED COMPONENTS Widgets Concurrent OPTIONAL_COMPONENTS DBus)
endif()

if (Qt5DBus_FOUND OR Qt6DBus_FOUND)
    add_compile_definitions(HAS_QTDBUS)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(qdeduper
    main.cpp
    mingui.cpp
    imageitem.cpp
    sigdb_qt.cpp
    filescanner.cpp
    pathchooser.cpp
    settings.cpp
    preferencedialog.cpp
    utilities.cpp
    resources.qrc
)

target_link_libraries(qdeduper
    xsig
)

if (QDEDUPER_USE_QT6)
    target_link_libraries(qdeduper Qt6::Widgets Qt6::Concurrent)
    if(Qt6DBus_FOUND)
        target_link_libraries(qdeduper Qt6::DBus)
    endif()
else()
    target_link_libraries(qdeduper Qt5::Widgets Qt5::Concurrent)
    if(Qt5DBus_FOUND)
        target_link_libraries(qdeduper Qt5::DBus)
    endif()
endif()

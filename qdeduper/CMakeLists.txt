set(CMAKE_INCLUDE_CURRENT_DIR ON)

option(QDEDUPER_USE_QT6 "Build qdeduper with Qt 6" OFF)

if (QDEDUPER_USE_QT6)
    find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)
else()
    find_package(Qt5 REQUIRED COMPONENTS Widgets Concurrent)
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
    resources.qrc
)

target_link_libraries(qdeduper
    xsig
)

if (QDEDUPER_USE_QT6)
    target_link_libraries(qdeduper Qt6::Widgets Qt6::Concurrent)
else()
    target_link_libraries(qdeduper Qt5::Widgets Qt5::Concurrent)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets Concurrent)

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
)

target_link_libraries(qdeduper
    xsig
    Qt5::Widgets
    Qt5::Concurrent
)

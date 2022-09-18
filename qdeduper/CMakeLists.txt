set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(qdeduper
    main.cpp
    mingui.cpp
    imageitem.cpp
)

target_link_libraries(qdeduper
    xsig
    Qt5::Widgets
)

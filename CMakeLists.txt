cmake_minimum_required(VERSION 3.16)
project(once VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network)


add_executable(once
    main.cpp
    qrc.qrc
)

target_link_libraries(once PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
)

install(TARGETS once DESTINATION bin)

cmake_minimum_required(VERSION 3.16)
project(once VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network)

# Create executable
add_executable(once
    main.cpp
    qrc.qrc
)

# Link Qt libraries
target_link_libraries(once PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Network
)

# Install target
install(TARGETS once DESTINATION bin)

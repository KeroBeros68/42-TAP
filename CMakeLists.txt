cmake_minimum_required(VERSION 3.5) # CMake version
project(MyQtApp LANGUAGES CXX) # C++

set(CMAKE_CXX_STANDARD 17) # C++ version
set(CMAKE_CXX_STANDARD_REQUIRED ON) # Force C++ version

# Essential for Qt to automatically handle MOC, Resources, and UI files
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED) # Find the Qt5 installation

add_executable(TAP_Client test/main.cpp)

# Link Qt5 libraries to your executable
target_link_libraries(TAP_Client Qt5::Widgets)
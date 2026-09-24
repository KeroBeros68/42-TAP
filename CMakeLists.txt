cmake_minimum_required(VERSION 3.5) # CMake version
project(MyQtApp LANGUAGES CXX) # C++

set(CMAKE_CXX_STANDARD 17) # 
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Essential for Qt to automatically handle MOC, Resources, and UI files
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find the Qt5 installation
find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable(MyQtApp test/main.cpp)

# Link Qt5 libraries to your executable
target_link_libraries(MyQtApp Qt5::Widgets)
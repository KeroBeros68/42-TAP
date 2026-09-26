cmake_minimum_required(VERSION 4.2) # CMake version
project(42TAP) # Project name

find_package(Qt5 COMPONENTS Widgets REQUIRED) # Find the Qt5 installation

add_executable(${PROJECT_NAME} test/main.cpp test/gui/connection_window/connection_window.cpp)

# Link Qt5 libraries to your executable
target_link_libraries(${PROJECT_NAME} Qt5::Widgets)

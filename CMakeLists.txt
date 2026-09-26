cmake_minimum_required(VERSION 3.3) # CMake version
project(42TAP) # Project name

find_package(Qt5 COMPONENTS Widgets REQUIRED) # Find the Qt5 installation

# Liste des executables a creer
add_executable(${PROJECT_NAME}
    test/main.cpp
    test/gui/connection_window/tap_connection_window.cpp
    test/gui/custom_ui_elements/tap_line_edit/tap_line_edit.cpp
    test/gui/custom_ui_elements/tap_label/tap_label.cpp
)
# ...

# Libraries a ajouter pour chaque executable
target_link_libraries(${PROJECT_NAME} Qt5::Widgets)
# ...

cmake_minimum_required(VERSION 3.2) # CMake version
project(42TAP) # Project name

# Include a compile_commands.json file in the build folder
#  in order to make the code editor resolve the unusual libraries
#  like qt5.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17) # Same standard on every compiler
set(CMAKE_CXX_STANDARD_REQUIRED ON) # Ragequit if the C++ version is not available

find_package(Qt5 COMPONENTS Widgets REQUIRED) # Find the Qt5 installation

# Liste des executables a creer
add_executable(${PROJECT_NAME}
    test/main.cpp

    test/gui/windows/base_window/tap_base_window.cpp
    test/gui/windows/connection_window/tap_connection_window.cpp
    test/gui/windows/error_window/tap_error_window.cpp
    test/gui/windows/game_window/tap_game_window.cpp

    test/gui/custom_ui_elements/tap_line_edit/tap_line_edit.cpp
    test/gui/custom_ui_elements/tap_label/tap_label.cpp
    test/gui/custom_ui_elements/tap_game_section_widget/tap_game_section_widget.cpp
)
# ...

# Libraries a ajouter pour chaque executable
target_link_libraries(${PROJECT_NAME} Qt5::Widgets)
# ...

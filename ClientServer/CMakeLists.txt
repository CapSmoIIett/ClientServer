# CMakeList.txt: проект CMake для ClientServer; включите исходный код и определения,
# укажите здесь логику для конкретного проекта.
#
cmake_minimum_required (VERSION 3.8)

#include_directories(&{PROJECT_SOURCE_DIR})

add_subdirectory ("Client")
add_subdirectory ("Server")
# TODO: Добавьте тесты и целевые объекты, если это необходимо.

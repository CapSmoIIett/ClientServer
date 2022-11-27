# CMakeList.txt: проект CMake для ClientServer; включите исходный код и определения,
# укажите здесь логику для конкретного проекта.
#
cmake_minimum_required (VERSION 3.8)

project ("Server")

if(WIN32)
    find_library(WSOCK32_LIBRARY wsock32)
    find_library(WS2_32_LIBRARY ws2_32)
endif()

# Добавьте источник в исполняемый файл этого проекта.
add_executable (Server "main.cpp" "../ConnectionTools/Headers/ConnectionInterface.h" "../ConnectionTools/Sources/TCPConnection.cpp" "../OSDefines.h" "../ConnectionTools/Headers/TCPConnection.h" "../ConnectionTools/Sources/ConnectionInterface.cpp" "../ConnectionTools/Headers/ClientServer.h" )

if(WIN32)
	target_link_libraries(Server wsock32 ws2_32)
endif()

# TODO: Добавьте тесты:w и целевые объекты, если это необходимо.

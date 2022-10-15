# CMakeList.txt: проект CMake для ClientServer; включите исходный код и определения,
# укажите здесь логику для конкретного проекта.
#
cmake_minimum_required (VERSION 3.8)

project ("Client")

if(WIN32)
	find_library(WSOCK32_LIBRARY wsock32)
    find_library(WS2_32_LIBRARY ws2_32)
endif()


# Добавьте источник в исполняемый файл этого проекта.
add_executable (Client "main.cpp" "../ConnectionTools/ConnectionInterface.h" "../ConnectionTools/TCPConnection.cpp" "../OSDefines.h" "../ConnectionTools/TCPConnection.h" "../ConnectionTools/ConnectionInterface.cpp" "../ConnectionTools/ClientServer.h")

if(WIN32)
	target_link_libraries(Client wsock32 ws2_32)
endif()

# TODO: Добавьте тесты и целевые объекты, если это необходимо.

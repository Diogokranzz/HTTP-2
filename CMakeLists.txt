cmake_minimum_required(VERSION 3.25)
project(UltraLowLatencyServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Werror -pedantic)
endif()

set(SRC_FILES
    src/main.cpp
    src/core/BufferPool.cpp
    src/http/Parser.cpp
)

if(WIN32)
    list(APPEND SRC_FILES src/sys/WindowsIOCP.cpp)
    add_compile_definitions(PLATFORM_WINDOWS)
elseif(UNIX)
    list(APPEND SRC_FILES src/sys/LinuxUring.cpp)
    add_compile_definitions(PLATFORM_LINUX)
    # User must install liburing manually or we vendor it
    # find_package(LibUring REQUIRED) 
endif()

add_executable(server ${SRC_FILES})

if(UNIX)
    target_link_libraries(server PRIVATE uring)
elseif(WIN32)
    target_link_libraries(server PRIVATE ws2_32 mswsock)
endif()

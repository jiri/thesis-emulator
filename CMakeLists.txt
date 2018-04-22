cmake_minimum_required(VERSION 3.10)

project(emulator)

set(CMAKE_CXX_STANDARD 17)

add_executable(${PROJECT_NAME} src/main.cpp)

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE src/)

target_link_libraries(${PROJECT_NAME} fmt)

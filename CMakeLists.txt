cmake_minimum_required(VERSION 3.10)

project(emulator)

set(CMAKE_CXX_STANDARD 17)

add_compile_options(-include ${CMAKE_CURRENT_SOURCE_DIR}/common/typedefs.hpp)

link_libraries(fmt)

# Library
set(SOURCE_FILES
        src/Mcu.hpp
        src/Mcu.cpp
        src/opcodes.hpp
)

add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE src/)

# Tests
add_executable(${PROJECT_NAME}_tests ${SOURCE_FILES} test/main.cpp)
target_include_directories(${PROJECT_NAME}_tests SYSTEM PRIVATE src/)

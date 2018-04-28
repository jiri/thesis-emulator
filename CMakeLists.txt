cmake_minimum_required(VERSION 3.10)

project(emulator)

set(CMAKE_CXX_STANDARD 17)

link_libraries(fmt)

# Library
set(SOURCE_FILES
        src/Mcu.hpp
        src/Mcu.cpp
        src/interrupts.hpp
        src/opcodes.hpp
        src/typedefs.hpp
)

add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC src/)

# Tests
set(TEST_FILES
        test/Mcu.cpp
)

add_executable(${PROJECT_NAME}_tests ${TEST_FILES} test/main.cpp)
target_include_directories(${PROJECT_NAME}_tests SYSTEM PRIVATE src/)
target_link_libraries(${PROJECT_NAME}_tests PUBLIC ${PROJECT_NAME})

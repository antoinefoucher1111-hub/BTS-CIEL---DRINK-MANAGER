cmake_minimum_required(VERSION 3.15)
project(DrinkManager VERSION 1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# SQLite embarquee (identique sur Windows et Raspberry Pi / Linux)
include_directories(include src sqlite)
file(GLOB_RECURSE SRC_FILES "src/*.cpp")
add_executable(drink_manager ${SRC_FILES} sqlite/sqlite3.c)

if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0a00)
    target_link_libraries(drink_manager ws2_32 crypt32)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(drink_manager Threads::Threads ${CMAKE_DL_LIBS})
endif()

cmake_minimum_required(VERSION 3.16)
project(DrinkManager LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Test)   # Qt5 : remplace Qt6 par Qt5
enable_testing()

add_executable(test_protocole
    tests/test_protocole.cpp
    src/materiel/protocole.cpp)
target_link_libraries(test_protocole PRIVATE Qt6::Core Qt6::Test)
add_test(NAME protocole COMMAND test_protocole)
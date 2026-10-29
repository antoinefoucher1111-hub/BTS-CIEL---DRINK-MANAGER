cmake_minimum_required(VERSION 3.16)
project(DrinkManager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SECURITE_DIR ${CMAKE_SOURCE_DIR}/src/auth/securite)

add_executable(MachineACafe
    ${SECURITE_DIR}/main.cpp
    ${SECURITE_DIR}/portefeuille.cpp
    ${SECURITE_DIR}/securite.cpp
)
target_include_directories(MachineACafe PRIVATE ${SECURITE_DIR})

enable_testing()

add_executable(test_jeton
    ${SECURITE_DIR}/tests/test_jeton.cpp
    ${SECURITE_DIR}/jeton.cpp
    ${SECURITE_DIR}/securite.cpp
)
target_include_directories(test_jeton PRIVATE ${SECURITE_DIR})
add_test(NAME test_jeton COMMAND test_jeton)

add_executable(test_authentificateur
    ${SECURITE_DIR}/tests/test_authentificateur.cpp
    ${SECURITE_DIR}/authentificateur.cpp
    ${SECURITE_DIR}/jeton.cpp
    ${SECURITE_DIR}/securite.cpp
)
target_include_directories(test_authentificateur PRIVATE ${SECURITE_DIR})
add_test(NAME test_authentificateur COMMAND test_authentificateur)

add_custom_target(tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_jeton test_authentificateur
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

cmake_minimum_required(VERSION 3.16)
project(DrinkManager VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dépendances système
find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)

# Définition de l'exécutable
add_executable(drink_manager
    src/main.cpp
    src/repositories/UserRepository.cpp
)

target_include_directories(drink_manager PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Liens avec les bibliothèques
target_link_libraries(drink_manager PRIVATE 
    SQLite::SQLite3 
    Threads::Threads
)
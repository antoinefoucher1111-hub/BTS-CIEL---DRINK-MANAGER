cmake_minimum_required(VERSION 3.15)
project(DrinkManager VERSION 1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Dossiers d'en-têtes principaux
include_directories(include src)

if(WIN32)
    message(STATUS "Système Windows détecté : Utilisation de SQLite3 en local (Amalgamation).")
    
    # Force MinGW à cibler Windows récent (nécessaire pour cpp-httplib)
    add_compile_definitions(_WIN32_WINNT=0x0a00)
    
    # Inclut le dossier sqlite pour trouver sqlite3.h
    include_directories(sqlite)

    # On compile sqlite3.c directement avec le projet
    add_executable(drink_manager 
        src/main.cpp 
        src/repositories/UserRepository.cpp
        sqlite/sqlite3.c
    )

    # Librairies réseau et système requises par cpp-httplib sous Windows
    target_link_libraries(drink_manager ws2_32 crypt32)

else()
    message(STATUS "Système Linux détecté : Utilisation du SQLite3 natif.")
    
    add_executable(drink_manager 
        src/main.cpp 
        src/repositories/UserRepository.cpp
    )

    find_package(SQLite3 REQUIRED)
    find_package(Threads REQUIRED)
    
    target_link_libraries(drink_manager SQLite::SQLite3 Threads::Threads)
endif()
cmake_minimum_required(VERSION 3.14)
project(ClassifierTests)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
)
FetchContent_MakeAvailable(googletest)

# Directorios de cabeceras (ajusta según tu proyecto)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../core
    ${CMAKE_CURRENT_SOURCE_DIR}/../nlp
    ${CMAKE_CURRENT_SOURCE_DIR}/../db
    ${CMAKE_CURRENT_SOURCE_DIR}/../dialogue
    ${CMAKE_CURRENT_SOURCE_DIR}/../utils
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${CMAKE_CURRENT_SOURCE_DIR}/../api
)

# Recopila todos los archivos fuente del proyecto (excepto main.cpp)
file(GLOB_RECURSE PROJECT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../nlp/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../db/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../dialogue/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../utils/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../api/*.cpp
)

# Elimina main.cpp si existe (para no tener conflictos con el main de pruebas)
list(REMOVE_ITEM PROJECT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../main.cpp)

# Archivos de prueba
set(TEST_SOURCES
    test_tokenizer.cpp
    test_morphology.cpp
    test_tagstats.cpp
    test_refiner.cpp
    test_classifier.cpp
    test_nlpengine.cpp
)

add_executable(unit_tests ${TEST_SOURCES} ${PROJECT_SOURCES})

target_link_libraries(unit_tests
    gtest_main
    sqlite3
    pthread
)

target_compile_definitions(unit_tests PRIVATE GTEST_HAS_PTHREAD=1)

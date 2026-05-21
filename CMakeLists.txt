cmake_minimum_required(VERSION 3.14)   # Necesario para FetchContent
project(NLPEngine VERSION 1.0)

# Opción para forzar la descarga de SQLite3 en lugar de usar la del sistema
option(FORCE_DOWNLOAD_SQLITE3 "Forzar descarga y compilación de SQLite3 desde fuente" OFF)

# Opciones para elegir qué construir
option(BUILD_EXECUTABLE   "Construir el ejecutable nlp_engine" ON)
option(BUILD_STATIC_LIB   "Construir biblioteca estática" OFF)
option(BUILD_SHARED_LIB   "Construir biblioteca dinámica (compartida)" OFF)

# Estándar C++17 obligatorio
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Configuración por defecto: Release si no se especifica
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Tipo de compilación" FORCE)
endif()

# Advertencias útiles (compatibles con GCC/Clang/MSVC)
if(MSVC)
    add_compile_options(/W4 /WX-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor)
endif()

# -------------------------------------------------------------------
# 1. Intentar encontrar SQLite3 en el sistema
# -------------------------------------------------------------------
set(SQLITE3_FOUND_SYSTEM FALSE)

if(NOT FORCE_DOWNLOAD_SQLITE3)
    find_package(SQLite3 QUIET)
    if(SQLite3_FOUND)
        message(STATUS "SQLite3 encontrado en el sistema: ${SQLite3_INCLUDE_DIRS}")
        set(SQLITE3_FOUND_SYSTEM TRUE)
    else()
        message(STATUS "SQLite3 no encontrado en el sistema. Se descargará y compilará.")
    endif()
endif()

# -------------------------------------------------------------------
# 2. Si no se encontró o se fuerza la descarga, obtener SQLite3 desde fuente
# -------------------------------------------------------------------
if(NOT SQLITE3_FOUND_SYSTEM OR FORCE_DOWNLOAD_SQLITE3)
    include(FetchContent)

    # Versión actual de SQLite3 (amalgamación) - actualizar si es necesario
    set(SQLITE3_VERSION "3460000")  # 3.46.0
    set(SQLITE3_YEAR "2024")
    set(SQLITE3_URL "https://www.sqlite.org/${SQLITE3_YEAR}/sqlite-amalgamation-${SQLITE3_VERSION}.zip")

    FetchContent_Declare(
        sqlite3
        URL ${SQLITE3_URL}
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    FetchContent_GetProperties(sqlite3)
    if(NOT sqlite3_POPULATED)
        FetchContent_Populate(sqlite3)
    endif()

    # Compilar SQLite3 como biblioteca estática
    add_library(sqlite3 STATIC "${sqlite3_SOURCE_DIR}/sqlite3.c")
    target_include_directories(sqlite3 PUBLIC "${sqlite3_SOURCE_DIR}")
    target_compile_definitions(sqlite3 PRIVATE SQLITE_THREADSAFE=1)

    set(SQLITE3_INCLUDE_DIRS "${sqlite3_SOURCE_DIR}")
    set(SQLITE3_LIBRARIES sqlite3)
    message(STATUS "SQLite3 compilado desde fuente en: ${sqlite3_SOURCE_DIR}")
else()
    set(SQLITE3_LIBRARIES SQLite::SQLite3)
endif()

# -------------------------------------------------------------------
# 3. Fuentes del proyecto
# -------------------------------------------------------------------
# Fuentes comunes de la biblioteca (todo excepto main.cpp)
set(LIB_SOURCES
    src/api/NLPEngine.cpp
    src/core/Command.cpp
    src/core/Dialogue.cpp
    src/core/InferenceEngine.cpp
    src/core/Pattern.cpp
    src/core/Sentence.cpp
    src/core/Word.cpp
    src/db/CleanerManager.cpp
    src/db/DatabaseManager.cpp
    src/db/DialogueRepository.cpp
    src/db/PatternRepository.cpp
    src/db/SentenceRepository.cpp
    src/db/TemplateRepository.cpp
    src/db/WordRepository.cpp
    src/dialogue/ChunkCorrelator.cpp
    src/dialogue/ContextualCorrelator.cpp
    src/dialogue/PatternCorrelator.cpp
    src/dialogue/LetterCorrelator.cpp
    src/dialogue/SyllableCorrelator.cpp
    src/nlp/Classifier.cpp
    src/nlp/Morphology.cpp
    src/nlp/Refiner.cpp
    src/nlp/TagStats.cpp
    src/nlp/Tokenizer.cpp
    src/utils/Chunker.cpp
    src/utils/LearningHelpers.cpp
    src/utils/PatternUtils.cpp
    src/utils/ResponseTemplates.cpp
    src/utils/SentenceUtils.cpp
    src/utils/SlotFiller.cpp
    src/utils/StringConversions.cpp
)

# Fuentes del ejecutable (main)
set(EXE_SOURCES main.cpp)

# -------------------------------------------------------------------
# 4. Biblioteca de objetos (compila una vez, reutiliza en estática/dinámica/ejecutable)
# -------------------------------------------------------------------
add_library(nlp_engine_objects OBJECT ${LIB_SOURCES})
target_include_directories(nlp_engine_objects PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(nlp_engine_objects ${SQLITE3_LIBRARIES})
# Necesario para poder incluir estos objetos en una biblioteca compartida
set_property(TARGET nlp_engine_objects PROPERTY POSITION_INDEPENDENT_CODE ON)

# -------------------------------------------------------------------
# 5. Construcción de bibliotecas estática y dinámica (opcional)
# -------------------------------------------------------------------
if(BUILD_STATIC_LIB)
    add_library(nlp_engine_static STATIC $<TARGET_OBJECTS:nlp_engine_objects>)
    target_link_libraries(nlp_engine_static ${SQLITE3_LIBRARIES})
    set_target_properties(nlp_engine_static PROPERTIES OUTPUT_NAME "nlp_engine")
    message(STATUS "Biblioteca estática habilitada: nlp_engine_static")
endif()

if(BUILD_SHARED_LIB)
    add_library(nlp_engine_shared SHARED $<TARGET_OBJECTS:nlp_engine_objects>)
    target_link_libraries(nlp_engine_shared ${SQLITE3_LIBRARIES})
    set_target_properties(nlp_engine_shared PROPERTIES OUTPUT_NAME "nlp_engine")
    message(STATUS "Biblioteca dinámica habilitada: nlp_engine_shared")
endif()

# -------------------------------------------------------------------
# 6. Ejecutable (opcional, enlaza directamente con los objetos)
# -------------------------------------------------------------------
if(BUILD_EXECUTABLE)
    add_executable(nlp_engine ${EXE_SOURCES} $<TARGET_OBJECTS:nlp_engine_objects>)
    target_link_libraries(nlp_engine ${SQLITE3_LIBRARIES})

    # Opción para enlace estático frente a dinámico (útil en despliegues)
    if(UNIX AND NOT APPLE AND NOT FORCE_DOWNLOAD_SQLITE3)
        target_link_options(nlp_engine PRIVATE -static-libstdc++ -static-libgcc)
    endif()
    message(STATUS "Ejecutable habilitado: nlp_engine")
endif()

# -------------------------------------------------------------------
# 7. Instalación (opcional, ajusta las rutas según tu estructura)
# -------------------------------------------------------------------
# Instalar bibliotecas
if(BUILD_STATIC_LIB)
    install(TARGETS nlp_engine_static
            ARCHIVE DESTINATION lib)
endif()
if(BUILD_SHARED_LIB)
    install(TARGETS nlp_engine_shared
            LIBRARY DESTINATION lib
            RUNTIME DESTINATION bin)  # Para Windows
endif()
if(BUILD_EXECUTABLE)
    install(TARGETS nlp_engine
            RUNTIME DESTINATION bin)
endif()

# Instalar cabeceras públicas (asumiendo que están en include/ o src/api)
install(DIRECTORY include/ DESTINATION include
        FILES_MATCHING PATTERN "*.hpp"
        PATTERN ".git*" EXCLUDE)
install(DIRECTORY src/api/ DESTINATION include/NLPEngine
        FILES_MATCHING PATTERN "*.hpp")

# -------------------------------------------------------------------
# Mensaje final de configuración
# -------------------------------------------------------------------
message(STATUS "===== Configuración completada =====")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
if(SQLITE3_FOUND_SYSTEM)
    message(STATUS "SQLite3: usando biblioteca del sistema")
else()
    message(STATUS "SQLite3: usando biblioteca compilada desde fuente")
endif()
message(STATUS "Opciones: Ejecutable=${BUILD_EXECUTABLE}, Estática=${BUILD_STATIC_LIB}, Dinámica=${BUILD_SHARED_LIB}")

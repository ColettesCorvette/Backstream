cmake_minimum_required(VERSION 3.15)
project(BackStream VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- CONFIGURATION PROPRE DU DOSSIER DE SORTIE ---
# On définit un dossier "Portable" à la racine de la construction
set(DIST_DIR ${CMAKE_BINARY_DIR}/Portable)

# On force Visual Studio à ne PAS ajouter /Release ou /Debug à la fin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DIST_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${DIST_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${DIST_DIR})

set(SOURCES
    src/main.cpp
    src/utils.cpp
    src/progress.cpp
    src/backup.cpp
    src/config.cpp
    src/resources.rc
)

set(HEADERS
    include/backup.h
    include/config.h
    include/progress.h
    include/utils.h
)

add_executable(backup ${SOURCES} ${HEADERS})
target_include_directories(backup PRIVATE include)

if(MSVC)
    target_compile_options(backup PRIVATE /EHsc /W3 /O2)
endif()

# --- COPIE DES FICHIERS NÉCESSAIRES ---

add_custom_command(TARGET backup POST_BUILD
    # Copie de zstd.exe
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/tools/zstd.exe"
    "${DIST_DIR}/zstd.exe"
    
    COMMENT "Creation du package final dans /Portable..."
)

message(STATUS ">>> BackStream build complete. Executable ready in: ${DIST_DIR}")
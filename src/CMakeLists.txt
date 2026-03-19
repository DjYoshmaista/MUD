# src/CMakeLists.txt
# Core MUD Lib

# -- Creates a variable listing all source files for the core lib
set(MUD_CORE_SOURCES
    log/mud_log.c
    utils/mud_crypto.c
    utils/mud_db.c
    utils/mud_json.c
    config/mud_config.c
    utils/mud_vector.c
    utils/mud_buffer.c
    utils/mud_str.c
    utils/mud_hashmap.c
    utils/mud_utils.c
    arena/mud_arena.c
    arena/mud_arena_temp.c
    arena/mud_arena_str.c
)

# -- Library definition
#       -- Creates a static lib named mud_core from listed sources
#       -- STATIC: Produces an archive file (libmud_core.a on POSIX) Code copied to executables at link time
add_library(mud_core STATIC ${MUD_CORE_SOURCES})

# -- Include directories: Includes the directory containing the source files
target_include_directories(mud_core PUBLIC ${PROJECT_SOURCE_DIR}/include)

# -- Compiler features: Modern CMake way to specify language requirements (per targt)
target_compile_features(mud_core PUBLIC c_std_23)

# -- Additional warning flags:
#       -- Adds extra warnings specific to this library
#       -- PRIVATE: Only applies when compiling this lib, not propagated to consumers
target_compile_options(mud_core PRIVATE
    -Wformat=2
    -Wformat-overflow=2
    -Wformat-truncation=2
)

target_link_libraries(mud_core PUBLIC
    deps::zlog
    deps::sodium
    SQLite::SQLite3
    deps::lcjson
    deps::lconfig
    Threads::Threads
)
# -- Library Alias: Creates a namespaced alias for the library
add_library(MUD::core ALIAS mud_core)

add_executable(mudv2 main.c)
target_link_libraries(mudv2 PRIVATE mud_core)
target_compile_features(mudv2 PRIVATE c_std_23)

# -- Status Message: Confirms which files are included during configuration
message(STATUS "Core library sources: ${MUD_CORE_SOURCES}")

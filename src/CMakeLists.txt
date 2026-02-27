# src/CMakeLists.txt
# Core MUD Lib - foundational data structures and utils

# ------------------------------------------------------------------------------
# Creates a variable listing all source files for the core library
# ------------------------------------------------------------------------------
set(MUD_CORE_SOURCES
    mud_vector.c
    mud_buffer.c
    mud_str.c
    mud_hashmap.c
)

# ------------------------------------------------------------------------------
# Library definition
#   - Creates a static library named mud_core from listed sources
#   - STATIC: Produces an archive file (libmud_core.a on Linux).  Code copied to
#	      executables at link time
# ------------------------------------------------------------------------------
add_library(mud_core STATIC ${MUD_CORE_SOURCES})

# ------------------------------------------------------------------------------
# Include directories:
#   - Includes the directory containing the source files
# ------------------------------------------------------------------------------
target_include_directories(mud_core PUBLIC ${CMAKE_SOURCE_DIR}/include)

# ------------------------------------------------------------------------------
# Compiler Features:
#   - Modern CMake way to specify language requirements (per target)
# ------------------------------------------------------------------------------
target_compile_features(mud_core PUBLIC c_std_23)

# ------------------------------------------------------------------------------
# Additional Warning Flags:
#   - Adds extra warninngs specific to this library
#   - PRIVATE: Only applies when compiling this library, not propagated to consumers
# ------------------------------------------------------------------------------
target_compile_options(mud_core PRIVATE
    -Wformat=2
    -Wformat-overflow=2
    -Wformat-truncation=2
)

# ------------------------------------------------------------------------------
# Library Alias
#   - Creates a namespaced alias for the library
# ------------------------------------------------------------------------------
add_library(MUD::core ALIAS mud_core)

# ------------------------------------------------------------------------------
# Status Message:
#   - Confirms which files are included during configuration
# ------------------------------------------------------------------------------
message(STATUS "Core library sources: ${MUD_CORE_SOURCES}")

# cmake/Deps.cmake
# Thrid party dependencies
# All find_package / pkg_check_modules calls here
# Consumers link against the interface targets defined at the bottom

# -- pkg-config is used for libraries that don't ship CMake config files
find_package(PkgConfig REQUIRED)

# -- zlog: structured logging with categories, multiple sinks, config-file rules
pkg_check_modules(ZLOG REQUIRED zlog)

if(ZLOG_FOUND)
    message(STATUS "zlog found: ${ZLOG_VERSION}")
    message(STATUS "  includes: ${ZLOG_INCLUDE_DIRS}")
    message(STATUS "  libs:     ${ZLOG_LIBRARIES}")
else()
    message(FATAL_ERROR "zlog not found.  Install w/ 'yay -S zlog-git'")
endif()

# Create an imported interface target so consumers just write:
#   target_link_libraries(my_target PRIVATE deps::zlog)
add_library(deps::zlog INTERFACE IMPORTED)
target_include_directories(deps::zlog INTERFACE ${ZLOG_INCLUDE_DIRS})
target_link_libraries(deps::zlog INTERFACE ${ZLOG_LIBRARIES})
target_compile_options(deps::zlog INTERFACE ${ZLOG_CFLAGS_OTHER})

# -- libsodium - cryptography: Argon2id password hashing, secure random, etcc
pkg_check_modules(LIBSODIUM REQUIRED libsodium)

if(LIBSODIUM_FOUND)
    message(STATUS "libsodium found: $[LIBSODIUM_VERSION}")
else()
    message(FATAL_ERROR "libsodium NOT found.  Install w/ pacman -S libsodium")
endif()

add_library(deps::sodium INTERFACE IMPORTED)
target_include_directories(deps::sodium INTERFACE ${LIBSODIUM_INCLUDE_DIRS})
target_link_libraries(deps::sodium INTERFACE ${LIBSODIUM_LIBRARIES})
target_compile_options(deps::sodium INTERFACE ${LIBSODIUM_CFLAGS_OTHER})

# -- SQLite3 - embedded relational database.  No pkg-config needed
find_package(SQLite3 REQUIRED)

if(SQLite3_FOUND)
    message(STATUS "SQLite3 found: ${SQLite3_VERSION}")
else()
    message(FATAL_ERROR "SQLite3 not found.  Install with: pacman -S sqlite")
endif()

# SQLit3 is alreadya  proper CMake imported target; no wrapper necessary
# Consumers write: target_link_libaaries(my_target PRIVATE SQLite::SQLite3)

# -- cJSON - Lightweight JSON parsing and generation
pkg_check_modules(CJSON REQUIRED libcjson)

if(CJSON_FOUND)
    message(STATUS "cJSON found: ${CJSON_VERSION}")
else()
    message(FATAL_ERROR "cJSON not found.  Install w/ pacman -S cjson")
endif()

add_library(deps::cjson INTERFACE IMPORTED)
target_include_directories(deps::cjson INTERFACE ${CJSON_INCLUDE_DIRS})
target_link_libraries(deps::cjson INTERFACE ${CJSON_LIBRARIES})
target_compile_options(deps::cjson INTERFACE ${CJSON_CFLAGS_OTHER})

# -- libconfig - structured configuration file parsing
pkg_check_modules(LIBCONFIG REQUIRED libconfig)

if(LIBCONFIG_FOUND)
    message(STATUS "libconfig found: ${LIBCONFIG_VERSION}")
else()
    message(FATAL_ERROR "libconfig not found.  Install w/ pacman -S libconfig")
endif()

add_library(deps::config INTERFACE IMPORTED)
target_include_directories(deps::config INTERFACE ${LIBCONFIG_INCLUDE_DIRS})
target_link_libraries(deps::config INTERFACE ${LIBCONFIG_LIBRARIES})
target_compile_options(deps::config INTERFACE ${LIBCONFIG_CFLAGS_OTHER})

# -- Threads (POSIX pthreads explicit)
find_package(Threads REQUIRED)      # Exposes Threads::Threads imported target

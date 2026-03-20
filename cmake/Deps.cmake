# cmake/Deps.cmake
# Thrid party dependencies
# All find_package / pkg_check_modules calls here
# Consumers link against the interface targets defined at the bottom

# -- pkg-config is used for libraries that don't ship CMake config files
find_package(PkgConfig REQUIRED)

# -- zlog: structured logging with categories, multiple sinks, config-file rules
find_path(ZLOG_INCLUDE_DIR zlog.h)
find_library(ZLOG_LIBRARY zlog)

if(NOT ZLOG_INCLUDE_DIR OR NOT ZLOG_LIBRARY)
    message(FATAL_ERROR "zlog not found. Install the zlog headers and library")
endif()

add_library(deps::zlog INTERFACE IMPORTED)
target_include_directories(deps::zlog INTERFACE ${ZLOG_INCLUDE_DIR})
target_link_libraries(deps::zlog INTERFACE ${ZLOG_LIBRARY})

# -- libsodium - cryptography: Argon2id password hashing, secure random, etcc
pkg_check_modules(LIBSODIUM REQUIRED libsodium)

if(LIBSODIUM_FOUND)
    message(STATUS "libsodium found: ${LIBSODIUM_VERSION}")
    add_library(deps::sodium INTERFACE IMPORTED)
    target_include_directories(deps::sodium INTERFACE ${LIBSODIUM_INCLUDE_DIRS})
    target_link_libraries(deps::sodium INTERFACE ${LIBSODIUM_LIBRARIES})
    target_compile_options(deps::sodium INTERFACE ${LIBSODIUM_CFLAGS_OTHER})
else()
    message(FATAL_ERROR "libsodium NOT found.  Install w/ pacman -S libsodium")
endif()

# -- libtelnet - telnet protocol implementation
pkg_check_modules(LIBTELNET REQUIRED libtelnet)

if(LIBTELNET_FOUND)
    message(STATUS "libtelnet found: ${LIBTELNET_VERSION}")
    add_library(deps::ltelnet INTERFACE IMPORTED)
    target_include_directories(deps::ltelnet INTERFACE ${LIBTELNET_INCLUDE_DIRS})
    target_link_libraries(deps::ltelnet INTERFACE ${LIBTELNET_LIBRARIES})
    target_compile_options(deps::ltelnet INTERFACE ${LIBTELNET_CFLAGS_OTHER})
else()
    message(FATAL_ERROR "libtelnet not found.  Install w/ pacman -S libtelnet")
endif()


# -- libuv - networking: async I/O, event loop, etc
# -- LibUV integration for networking layer and async I/O
pkg_check_modules(LIBUV QUIET libuv)

if(LIBUV_FOUND)
    add_library(deps::luv INTERFACE IMPORTED)
    target_include_directories(deps::luv INTERFACE ${LIBUV_INCLUDE_DIRS})
    target_link_libraries(deps::luv INTERFACE ${LIBUV_LIBRARIES})
    target_compile_options(deps::luv INTERFACE ${LIBUV_CFLAGS_OTHER})
endif()

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
    add_library(deps::lcjson INTERFACE IMPORTED)
    target_include_directories(deps::lcjson INTERFACE ${CJSON_INCLUDE_DIRS})
    target_link_libraries(deps::lcjson INTERFACE ${CJSON_LIBRARIES})
    target_compile_options(deps::lcjson INTERFACE ${CJSON_CFLAGS_OTHER})
else()
    message(FATAL_ERROR "cJSON not found.  Install w/ pacman -S cjson")
endif()

# -- libconfig - structured configuration file parsing
pkg_check_modules(LIBCONFIG REQUIRED libconfig)

if(LIBCONFIG_FOUND)
    message(STATUS "libconfig found: ${LIBCONFIG_VERSION}")
    add_library(deps::lconfig INTERFACE IMPORTED)
    target_include_directories(deps::lconfig INTERFACE ${LIBCONFIG_INCLUDE_DIRS})
    target_link_libraries(deps::lconfig INTERFACE ${LIBCONFIG_LIBRARIES})
    target_compile_options(deps::lconfig INTERFACE ${LIBCONFIG_CFLAGS_OTHER})
else()
    message(FATAL_ERROR "libconfig not found.  Install w/ pacman -S libconfig")
endif()

# -- Threads (POSIX pthreads explicit)
find_package(Threads REQUIRED)      # Exposes Threads::Threads imported target

# ------------------------------------------------------------------------------
# Exits if sanitizers are not requested
# ------------------------------------------------------------------------------
if(NOT MUD_ENABLE_SANITIZERS)
    return()
endif()

# ------------------------------------------------------------------------------
# Ensures if sanitizers are enabled, the user specified which ones
# ------------------------------------------------------------------------------
if(NOT DEFINED MUD_SANITIZERS OR MUD_SANITIZERS STREQUAL "")
    message(FATAL_ERROR "MUD_ENABLE_SANITIZERS is ON but MUD_SANITIZERS is not set")
endif()

# ------------------------------------------------------------------------------
# Converts "address,undefined" into a CMake list "address;undefined"
# ------------------------------------------------------------------------------
string(REPLACE "," ";" SANITIZER_LIST "${MUD_SANITIZERS}")

# ------------------------------------------------------------------------------
# Checks if both TSan and ASan were requested
# ------------------------------------------------------------------------------
list(FIND SANITIZER_LIST "thread" _has_tsan)
list(FIND SANITIZER_LIST "address" _has_asan)

if(_has_tsan GREATER -1 AND _has_asan GREATER -1)
    message(FATAL_ERROR "ThreadSanitizer and AddressSanitizer cannot be used together")
endif()

# ------------------------------------------------------------------------------
# Consturcts -fsanitize=address -fsanitize=undefined from the list
# ------------------------------------------------------------------------------
set(SANITIZER_FLAGS "")
foreach(san IN LISTS SANITIZER_LIST)
    list(APPEND SANITIZER_FLAGS "-fsanitize=${san}")
endforeach()

# ------------------------------------------------------------------------------
# Adds the sanitizer flags to all subsequent compilation and linking commands
# ------------------------------------------------------------------------------
add_compile_options(${SANITIZER_FLAGS})
add_link_options(${SANITIZER_FLAGS})

# ------------------------------------------------------------------------------
# Sanitizers output hexadecimal addresses in error messages warning
# Must use 'Debug' or 'RelWithDebInfo' builds to get fn name & line no output
# ------------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    message(WARNING "Sanitizers work best with debug info.  Consider using Debug or RelWithDebInfo builds.")
endif()

# ------------------------------------------------------------------------------
# Define CMake cache variables that can be passed to test via environment
# ------------------------------------------------------------------------------

# LeakSanitizer is disabled by default because this environment runs tests under
# supervision that conflicts with LSan's ptrace requirements.
set(ASAN_OPTIONS "detect_leaks=0:abort_on_error=0:halt_on_error=0" CACHE STRING "ASan runtime options")
# print_stacktrace=1 Show call stack on UBSan errors
set(UBSAN_OPTIONS "print_stacktrace=1:halt_on_error=0" CACHE STRING "UBSan runtime options")

# ------------------------------------------------------------------------------
# Status Messages: Prints during configuration so youc can verify what's enabled
# ------------------------------------------------------------------------------
message(STATUS "Sanitizers Enabled: ${MUD_SANITIZERS}")
message(STATUS "Sanitizer Flags: ${SANITIZER_FLAGS}")

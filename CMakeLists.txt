cmake_minimum_required(VERSION 3.23)

project(MUDv2
	VERSION 0.1.0
	DESCRIPTION "Second attempt at creating a MUD (Mult-User Dungeon) game engine"
	LANGUAGES C
)

# -- CMake Compile Definitions --
if(UNIX OR APPLE)
	# Enabel POSIX-1-2008 for fileno, isatty and getcwd
	add_compile_definitions(_POSIX_C_SOURCE=200809L)

	# For GNU extensions
	add_compile_definitions(_GNU_SOURCE)
endif()

# Windows: Suppress MSVC warnings about 'unsafe' functions
if(WIN32)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

# -- C Language Standard --
set(CMAKE_C_STANDARD			23)
set(CMAKE_C_STANDARD_REQUIRED	ON)
set(CMAKE_C_EXTENSIONS			OFF)

# -- Compiler Diagnostics --
add_compile_options(
	-Wall
	-Wextra
	-Wpedantic
	-Wconversion
	-Wshadow
	-Wundef
	-Wwrite-strings
	-Wmissing-prototypes
	-Wstrict-prototypes
	-Werror
)

# -- cmake/ Module Inclusion --
include(cmake/Sanitizers.cmake)
include(cmake/Coverage.cmake)
include(cmake/Deps.cmake)

# -- Public Header Search Path --
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

# -- Subsystem --
# Core Library - Always Built
add_subdirectory(src/core)

if(MUD_ENABLE_TESTS)
	include(CTest)
	add_subdirectory(test)
endif()

if(MUD_ENABLE_TOOLS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tools/CMakeLists.txt")
	add_subdirectory(tools)
endif()

if(MUD_ENABLE_SAMPLES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/samples/CMakeLists.txt")
	add_subdirectory(samples)
endif()

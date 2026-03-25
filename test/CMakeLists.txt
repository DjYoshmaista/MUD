# ------------------------------------------------------------------------------
# Test Utility Library: Common Test Utilities
#   - Header-only test framework
# ------------------------------------------------------------------------------
add_library(mud_test_lib INTERFACE)
target_include_directories(mud_test_lib INTERFACE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/test
)
set(MUD_TEST_FIXTURE_DB_PATH "${CMAKE_CURRENT_BINARY_DIR}/test_fixture.db")
target_compile_definitions(mud_test_lib INTERFACE
    MUD_TEST_FIXTURE_DB_PATH=\"${MUD_TEST_FIXTURE_DB_PATH}\"
)

# ------------------------------------------------------------------------------
# Linking Test Library
#   - Links the test executable againsst:
#     - mud_core: The MUD core library
# ------------------------------------------------------------------------------
target_link_libraries(mud_test_lib INTERFACE
    mud_core
)

add_library(mud_test_support STATIC
    test_registry.c
)
target_link_libraries(mud_test_support PUBLIC mud_test_lib)

add_executable(generate_test_db
    generate_test_db.c
)
target_link_libraries(generate_test_db PRIVATE SQLite::SQLite3)

# ------------------------------------------------------------------------------
# List All Test Module Soruces - One file per module being tested
# ------------------------------------------------------------------------------
set(TEST_MODULE_SOURCES
    modules/test_vector.c
    modules/test_crypto.c
    modules/test_json.c
    modules/test_db.c
    modules/test_config.c
    modules/test_buffer.c
    modules/test_str.c
    modules/test_hashmap.c
    modules/test_utils.c
    modules/test_arena.c
    modules/test_connection.c
    modules/test_input.c
    modules/test_output.c
    modules/test_net_loop.c
)

# ------------------------------------------------------------------------------
# Test Runner Executable
#   - Defines an exec tgt that compiles and links test runner w/ module tests
# ------------------------------------------------------------------------------
add_executable(run_all_tests
    run_all_tests.c
    ${TEST_MODULE_SOURCES}
)

# ------------------------------------------------------------------------------
# Linking Test Library
#   - Links the test executable against our test util lib
# ------------------------------------------------------------------------------
target_link_libraries(run_all_tests PRIVATE 
    mud_test_support
)

# ------------------------------------------------------------------------------
# Registering with CTest: Basic Registration
#   - Registers the test exec with CTest
#   - add_test(NAME <name> COMMAND <command>): Creates a CTest entry
#   - NAME: Identifier used in ctest -R <reges> filtering and reports
#   - COMMAND: What to exec.  Can be tgt name or arbitrary cmd
# ------------------------------------------------------------------------------
add_test(
    NAME run_all_tests
    COMMAND run_all_tests
)

add_test(
    NAME setup_test_db_fixture
    COMMAND generate_test_db
            ${CMAKE_SOURCE_DIR}/data/db_dest/test_fixture.sql
            ${MUD_TEST_FIXTURE_DB_PATH}
)
set_tests_properties(setup_test_db_fixture PROPERTIES
    FIXTURES_SETUP TestDatabaseFixture
    LABELS "fixture"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# ------------------------------------------------------------------------------
# Test Properties: Labels
#   - Attaches metadata labels to the test
#   - set_tests_properties(<test> PROPERTIES <prop> <val>): Sets props on reg test
#   - LABELS: Semicolon-separated list of labels - Used for filtering:
#      - ctest -L unit: run tests w/ "unit" label
#      - ctest -LE slow: exclude tests w/ "slow" label
# ------------------------------------------------------------------------------
set_tests_properties(run_all_tests PROPERTIES
    LABELS "unit;all"
    TIMEOUT 600
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# -------------------------------------------------------------------------------
# Per-Module Executables: Each module can run independently.  Enables parallel testing
#   - Each module can run independently
#   - Allows parallel testing
#   - run_single_module.c - A simple runner for individual modules (TODO: Create)
# -------------------------------------------------------------------------------
foreach(test_source IN LISTS TEST_MODULE_SOURCES)
    # Extract filename without extension
    get_filename_component(test_name ${test_source} NAME_WE)

    # Create executable
    add_executable(${test_name}
        run_single_module.c
        ${test_source}
    )

    target_link_libraries(${test_name} PRIVATE mud_test_support)

    # Register with CTest
    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES
        LABELS "unit"
        TIMEOUT 300
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        ENVIRONMENT "MUD_TEST_VERBOSE=1;MUD_LOG_LEVEL=trace"
    )
endforeach()

set(MUD_VERIFY_BUILD_MODE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyBuildMode.cmake")

add_test(
    NAME verify_debug_build_mode
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
        -DBUILD_DIR=${CMAKE_BINARY_DIR}/verify/debug
        -DBUILD_TYPE=Debug
        -P ${MUD_VERIFY_BUILD_MODE_SCRIPT}
)
set_tests_properties(verify_debug_build_mode PROPERTIES
    LABELS "verification;buildmode;debug"
    TIMEOUT 1800
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME verify_asan_ubsan_build_mode
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
        -DBUILD_DIR=${CMAKE_BINARY_DIR}/verify/asan
        -DBUILD_TYPE=Debug
        -DENABLE_SANITIZERS=ON
        -DSANITIZERS=address,undefined
        -P ${MUD_VERIFY_BUILD_MODE_SCRIPT}
)
set_tests_properties(verify_asan_ubsan_build_mode PROPERTIES
    LABELS "verification;buildmode;asan;ubsan"
    TIMEOUT 1800
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0:abort_on_error=0:halt_on_error=0;UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=0"
)

add_test(
    NAME verify_coverage_build_mode
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
        -DBUILD_DIR=${CMAKE_BINARY_DIR}/verify/coverage
        -DBUILD_TYPE=Debug
        -DENABLE_COVERAGE=ON
        -DRUN_COVERAGE_TARGET=ON
        -P ${MUD_VERIFY_BUILD_MODE_SCRIPT}
)
set_tests_properties(verify_coverage_build_mode PROPERTIES
    LABELS "verification;buildmode;coverage"
    TIMEOUT 2400
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

set_tests_properties(test_db PROPERTIES
    FIXTURES_REQUIRED TestDatabaseFixture
)

set_tests_properties(run_all_tests PROPERTIES
    FIXTURES_REQUIRED TestDatabaseFixture
)

# -------------------------------------------------------------------------------
# build_tests target: ninja build_tests compiles everything without executing
# -------------------------------------------------------------------------------
# Custom target to build all tests w/o running them
add_custom_target(build_tests)
add_dependencies(build_tests run_all_tests)
add_dependencies(build_tests generate_test_db)
foreach(test_source IN LISTS TEST_MODULE_SOURCES)
    get_filename_component(test_name ${test_source} NAME_WE)
    add_dependencies(build_tests ${test_name})
endforeach()

# Status output
message(STATUS "Test configuration complete")
message(STATUS "  Main runner: run_all_tests")
message(STATUS "  Module tests: ${TEST_MODULE_SOURCES}")

# ------------------------------------------------------------------------------
# Conditional Test Registration
#   - Only builds/registers slow tests when explicitly enabled
# ------------------------------------------------------------------------------
if(MUD_ENABLE_SLOW_TESTS)
    add_executable(test_slow_integration slow/test_integration.c)
    target_link_libraries(test_slow_integration PRIVATE mud_test_support)
    add_test(NAME test_slow_integration COMMAND test_slow_integration)
    set_tests_properties(test_slow_integration PROPERTIES
        LABELS "integration;slow"
        TIMEOUT 900
    )
endif()

# ------------------------------------------------------------------------------
# Working Directory for Tests
#   - Sets the current dir when the tests run
# ------------------------------------------------------------------------------
set_tests_properties(run_all_tests PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# ------------------------------------------------------------------------------
# Environment Variables for Tests
#   - Sets environment variables when the test runs
# ------------------------------------------------------------------------------
set_tests_properties(run_all_tests PROPERTIES
    ENVIRONMENT "MUD_TEST_VERBOSE=1;MUD_LOG_LEVEL=trace"
)

# ------------------------------------------------------------------------------
# Fixture Dependencies (Advanced)
#   - Defines setup/cleanup that runs before/after tests requiring a fixture
#   - FIXTURES_SETUP: Sets up the named fixture
#   - FIXTURES_CLEANUP: Cleans up the named fixture
#   - FIXTURES_REQUIRED: The named fixture is required for the test
# ------------------------------------------------------------------------------
# add_test(NAME setup_test_db COMMAND ${CMAKE_COMMAND} -E echo "Setting up...")
# set_tests_properties(setup_test_db PROPERTIES FIXTURES_SETUP TestDatabase)
# 
# add_test(NAME cleanup_test_db COMMAND ${CMAKE_COMMAND} -E echo "Cleaning up...")
# set_tests_properties(cleanup_test_db PROPERTIES FIXTURES_CLEANUP TestDatabase)
# 
# set_tests_properties(test_with_db PROPERTIES FIXTURES_REQUIRED TestDatabase)

# ------------------------------------------------------------------------------
# Custom Target: tests (Build All Test Executables)
#   - Creates a target that builds all test executables without running them
#   - add_custom_target(test): Creates a "phony" target (no output file)
#   - add_dependencies(tests run_all_test): Makes tests depend on run_all_tests
# ------------------------------------------------------------------------------
add_custom_target(tests)
add_dependencies(tests run_all_tests)
add_dependencies(tests generate_test_db)
foreach(test_source IN LISTS TEST_MODULE_SOURCES)
    get_filename_component(test_name ${test_source} NAME_WE)
    add_dependencies(tests ${test_name})
endforeach()

add_custom_target(verify_build_modes
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L verification
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# ------------------------------------------------------------------------------
# Status Message
#   - Reports test configuration during CMake run
# ------------------------------------------------------------------------------

# tests/CMakeLists.txt
# Test suite for the interactive MUD project/game

# Require CTest functionality
include(CTest)

# -- Minimum Version Guard
cmake_minimum_required(VERSION 3.23)

# -- Test Utility Library: Common Test Utils [Header-Only Test Framework]
add_library(mud_test_lib INTERFACE)
target_include_directories(mud_test_lib INTERFACE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/test
)

# -- Linking Test Library
target_link_libraries(mud_test_lib INTERFACE
    mud_core
)

add_library(mud_test_support STATIC
    test_registry.c
)
target_link_libraries(mud_test_support PUBLIC mud_test_lib)

# -- List All Test Module Sources - One file per module being tested
set(TEST_MODULE_SOURCES
    modules/test_utils.c
    modules/test_buffer.c
    modules/test_networking.c
    modules/test_arenas.c
    modules/test_auth.c
    modules/test_characters.c
    modules/test_logging.c
)

# -- Test Runner Executable: Defines an exec target that compiles and links test runner w/ module tests
add_executable(run_all_tests
    run_all_tests.c
    ${TEST_MODULE_SOURCES}
)

# -- Linking Test Library: Links the test executable against test util lib
target_link_libraries(run_all_tests PRIVATE
    mud_test_support
)

# -- Registering with CTest: Basic Registration
#       - Registers the test exec with CTest
#       - add_test(NAME <name> COMMAND <command>): Creates a CTest entry
#       - NAME: Identifier used in ctest -R <regex> filtering and reports
#       - COMMAND: What to exec.  Can be target name or arbitrary command
add_test(
    NAME run_all_tests
    COMMAND run_all_tests
)

# -- Test Properties: Labels
#       - Attaches metadata labels to the test
#       - set_tests_properties(<test> PROPERTIES <prop> <val>): Sets props on reg test
#       - LABELS: Semicolon-separated list of labels - Used for filtering:
#           - ctest -L unit: run tests w/ "unit" label
#           - ctest -LE slow: exclude tests w/ "slow" label
set_tests_properties(run_all_tests PROPERTIES
    LABELS "unit;all"
    TIMEOUT 300
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# -- Per-Module Executables: Each module can run independently.  Enables parallel testing
#       - Each module can run independently
#       - Allows Parallel Testing
#       - run_single_moudle.c: A simple runner for individual modules (TODO: CREATE)
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
    )
endforeach()

# -- build_tests target: Ninja build_tests compiles everything without executing
# Custom target to build all tests w/o running them
add_custom_target(build_tests)
add_dependencies(build_tests run_all_tests)
foreach(test_source IN LISTS TEST_MODULE_SOURCES)
    get_filename_component(test_name ${test_source} NAME_WE)
    add_dependencies(build_tests ${test_name})
endforeach()

# Status Output
message(STATUS "Test configuration complete")
message(STATUS "  Main Runner: run_all_tests")
message(STATUS "  Module tests: ${TEST_MODULE_SOURCES}")

# -- Conditional Test Registration: Only builds/registers slow tests when explicitly enabled
if(MUD_ENABLE_SLOW_TESTS)
    add_executable(test_slow_integration slow/test_integration.c)
    target_link_libraries(test_sloow_integration PRIVATE mud_test_support)
    add_test(NAME test_slow_integration COMMAND test_slow_integration)
    set_tests_properties(test_slow_integration PROPERTIES
        LABELS "integration;slow"
        TIMEOUT 600
    )
endif()

# -- Working Dir For Tests: Sets the current dir when the tests run
set_tests_properties(run_all_tests PROPERTIES
    WORKING_DIRECTORY #{CMAKE_SOURCE_DIR}
)

# -- Environment Variables for Tests: Sets environment variables when the tests runs
set_tests_properties(run_all_tests PROPERTIES
    ENVIRONMENT "MUD_TEST_VERBOSE=1;MUD_LOG_LEVEL=debug"
)

# -- Fixture Dependencies (advanced)
#       - Defines setup/cleanup that runs before/after tests requiring a fixture
#       - FIXTURES_SETUP: Sets up the named fixture
#       - FIXTURES_CLEANUP: Cleans up the named fixture
#       - FIXTURES_REQUIRED: The named fixture is required for the test
# -- TODO: IMPLEMENT
# add_test(NAME setup_test_db COMMAND ${CMAKE_COMMAND} -E echo "Setting up...")
# set_tests_properties(setup-test_db PROPERTIES FIXTURES_SETUP TestDatabase)
#
# add_test(NAME cleanup_test_db COMMAND ${CMAKE_COMMAND} -E echo "Cleaning up..")
# set_tests_properties(cleanup-test_db PROPERTIES FIXTURES_CLEANUP TestDatabase)
#
# set_tests_properties(test_with_db PROPERTIES FIXTURES_REQUIRED TestDatabase)

# -- Custom Target: tests (Build All Test Executables):
#       - Creates a target that builds all test executables without running them
#       - add_custom_target(tests): Creates a "phony" target (no output file)
#       - add_dependencies(tests run-all_tests): Makes tests depend on run_all_tests
add_custom_target(tests)
add_dependencies(tests run_all_tests)

# -- Status Message: Reports tests configuration during CMake run

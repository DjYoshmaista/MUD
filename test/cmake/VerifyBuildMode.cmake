if(NOT DEFINED SOURCE_DIR OR SOURCE_DIR STREQUAL "")
    message(FATAL_ERROR "SOURCE_DIR is required")
endif()

if(NOT DEFINED BUILD_DIR OR BUILD_DIR STREQUAL "")
    message(FATAL_ERROR "BUILD_DIR is required")
endif()

if(NOT DEFINED BUILD_TYPE OR BUILD_TYPE STREQUAL "")
    set(BUILD_TYPE "Debug")
endif()

if(NOT DEFINED ENABLE_SANITIZERS)
    set(ENABLE_SANITIZERS OFF)
endif()

if(NOT DEFINED ENABLE_COVERAGE)
    set(ENABLE_COVERAGE OFF)
endif()

if(NOT DEFINED RUN_COVERAGE_TARGET)
    set(RUN_COVERAGE_TARGET OFF)
endif()

set(configure_cmd
    ${CMAKE_COMMAND}
    -S "${SOURCE_DIR}"
    -B "${BUILD_DIR}"
    -G Ninja
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE}
    -DBUILD_TESTING=ON
    -DMUD_ENABLE_TESTS=ON
    -DMUD_ENABLE_TOOLS=OFF
    -DMUD_ENABLE_SAMPLES=OFF
    -DMUD_ENABLE_SANITIZERS=${ENABLE_SANITIZERS}
    -DMUD_ENABLE_COVERAGE=${ENABLE_COVERAGE}
)

if(ENABLE_SANITIZERS)
    if(NOT DEFINED SANITIZERS OR SANITIZERS STREQUAL "")
        message(FATAL_ERROR "SANITIZERS must be set when ENABLE_SANITIZERS=ON")
    endif()
    list(APPEND configure_cmd "-DMUD_SANITIZERS=${SANITIZERS}")
endif()

execute_process(
    COMMAND ${configure_cmd}
    RESULT_VARIABLE configure_rc
    OUTPUT_VARIABLE configure_out
    ERROR_VARIABLE configure_err
)
if(NOT configure_rc EQUAL 0)
    message(FATAL_ERROR "Configure failed\n${configure_out}\n${configure_err}")
endif()

execute_process(
    COMMAND ${CMAKE_COMMAND} --build "${BUILD_DIR}"
    RESULT_VARIABLE build_rc
    OUTPUT_VARIABLE build_out
    ERROR_VARIABLE build_err
)
if(NOT build_rc EQUAL 0)
    message(FATAL_ERROR "Build failed\n${build_out}\n${build_err}")
endif()

if(RUN_COVERAGE_TARGET)
    execute_process(
        COMMAND ${CMAKE_COMMAND} --build "${BUILD_DIR}" --target coverage
        RESULT_VARIABLE coverage_rc
        OUTPUT_VARIABLE coverage_out
        ERROR_VARIABLE coverage_err
    )
    if(NOT coverage_rc EQUAL 0)
        message(FATAL_ERROR "Coverage target failed\n${coverage_out}\n${coverage_err}")
    endif()
else()
    execute_process(
        COMMAND ${CMAKE_CTEST_COMMAND} --test-dir "${BUILD_DIR}" --output-on-failure -LE verification
        RESULT_VARIABLE test_rc
        OUTPUT_VARIABLE test_out
        ERROR_VARIABLE test_err
    )
    if(NOT test_rc EQUAL 0)
        message(FATAL_ERROR "CTest failed\n${test_out}\n${test_err}")
    endif()
endif()

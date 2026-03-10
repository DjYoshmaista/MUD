if(NOT MUD_ENABLE_COVERAGE)
    message(STATUS "Code Coverage is disabled")
    return()
endif()

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug"
   AND NOT CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo"
   AND NOT CMAKE_BUILD_TYPE STREQUAL "Coverage")
    message(WARNING
	"Coverage is enabled but build type is '${CMAKE_BUILD_TYPE}'. "
	"Code Coverage can only be enabled in Debug, RelWithDebInfo or Coverage builds. "
	"Consider using the 'coverage' preset which sets Debug mode."
    )
endif()

# ------------------------------------------------------------------------------
# Compiler/Linker Flag Addition:
#   - Adds coverage instrumentation to all compilations and links
# ------------------------------------------------------------------------------
add_compile_options(--coverage)
add_link_options(--coverage)

# ------------------------------------------------------------------------------
# Finding Required Tools:
#   - Locates the coverage tools on the system
# ------------------------------------------------------------------------------
find_program(GCOV_PATH gcov)
find_program(LCOV_PATH lcov)
find_program(GENHTML_PATH genhtml)

# ------------------------------------------------------------------------------
# Tool Availability Check:
#   - Checks which tools avail & sets flag for cond tgt creation
# ------------------------------------------------------------------------------
if(NOT GCOV_PATH)
    message(WARNING "gcov not found. Coverage data collection will fail.")
endif()

if(NOT LCOV_PATH OR NOT GENHTML_PATH)
    message(STATUS "locv/genhtml not found.  HTML report generation disabled.")
    set(MUD_COVERAGE_HTML_ENABLED FALSE)
else()
    set(MUD_COVERAGE_HTML_ENABLED TRUE)
endif()

# ------------------------------------------------------------------------------
# Coverage Output Directory:
#   - Defines where coverage reports will be generated
# ------------------------------------------------------------------------------
set(COVERAGE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/coverage")

# ------------------------------------------------------------------------------
# Custom Target: coverage-clean
#   - Creates a target that removes all coverage data
#   - add_custom_targe; Creats a named target that runs cmds but doesn't prod files
#   - ${CMAKE_COMMAND} -E rm -rf: CMake's cross-platofrm file removlal
# 				  -E invokes CMake's cmd-line tool mode
#   - find .. -name "*.gcda" -delete": Removes runtime coverage data files
#   - COMMENT: Message printed when target runs
#   - VERBATIM: Ensures arguments are passed exactly as written
#   - Usage: ninja coverage-clean or make coverage-clean
# ------------------------------------------------------------------------------
add_custom_target(coverage-clean
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${COVERAGE_OUTPUT_DIR}"
    COMMAND find "${CMAKE_BINARY_DIR}" -name "*.gcda" -delete
    COMMENT "Cleaning coverage data"
    VERBATIM
)

# ------------------------------------------------------------------------------
# Custom Target: coverage-baseline (lcov)
#   - Captures "zero-coverage" baseline--what instr code looks like before exec
#   - --capture --initial: lcov flags meaning "capture data, but treat as
#			   initial/baseline (all zeros)"
#   - --directory: Where to look for .gcno files (compile-time structure)
#   - --output-file: Where to write the .info file (lcov's intermediate format)
# ------------------------------------------------------------------------------
if(MUD_COVERAGE_HTML_ENABLED)
    add_custom_target(coverage-baseline
	COMMAND ${CMAKE_COMMAND} -E make_directory "${COVERAGE_OUTPUT_DIR}"
	COMMAND ${LCOV_PATH}
	    --capture
	    --initial
	    --directory "${CMAKE_BINARY_DIR}"
	    --output-file "${COVERAGE_OUTPUT_DIR}/baseline.info"
	COMMENT "Capturing baseline coverage (before tests)"
	VERBATIM
    )
endif()

# ------------------------------------------------------------------------------
# Custom Target: coverage-capture
#   - Captures actual coverage data after tests have run
#   - No --initial flag, this reads the .gcda files populated during test exec
# ------------------------------------------------------------------------------
if(MUD_COVERAGE_HTML_ENABLED)
    add_custom_target(coverage-capture
	COMMAND ${LCOV_PATH}
	    --capture
	    --directory "${CMAKE_BINARY_DIR}"
	    --output-file "${COVERAGE_OUTPUT_DIR}/test.info"
	COMMENT "Capturing test coverage data"
	VERBATIM
    )
endif()

# ------------------------------------------------------------------------------
# Custom Target: coverage-combine 
#   - Merges baseline and test data
# ------------------------------------------------------------------------------
if(MUD_COVERAGE_HTML_ENABLED)
    add_custom_target(coverage-combine
	COMMAND ${LCOV_PATH}
	    --add-tracefile "${COVERAGE_OUTPUT_DIR}/baseline.info"
	    --add-tracefile "${COVERAGE_OUTPUT_DIR}/test.info"
	    --output-file "${COVERAGE_OUTPUT_DIR}/combined.info"
	COMMENT "Combining baseline and test coverage"
	VERBATIM
    )
endif()

# ------------------------------------------------------------------------------
# Custom Target: coverage-filter
#   - Removes coverage data for files we don't care about
# ------------------------------------------------------------------------------
if(MUD_COVERAGE_HTML_ENABLED)
    add_custom_target(coverage-filter
	COMMAND ${LCOV_PATH}
	    --remove "${COVERAGE_OUTPUT_DIR}/combined.info"
	    "/usr/*"
	    "${CMAKE_BINARY_DIR}/*"
	    --output-file "${COVERAGE_OUTPUT_DIR}/filtered.info"
	COMMENT "Filtering out system and generated files"
	VERBATIM
    )
endif()

# ------------------------------------------------------------------------------
# Custom Target: coverage-html
#   - Generates browsable HTML report from filtered coverage data
#   - `genhtml` flags:
#     - --output-direcotry: Where to write the HTML files
#     - --title: Report title shown in browser
#     - --legend: Include color legend explaining coverage percentages
#     - --show-details: Show line-by-line coverage in source view
# ------------------------------------------------------------------------------
if(MUD_COVERAGE_HTML_ENABLED)
    add_custom_target(coverage-html
	COMMAND ${GENHTML_PATH}
	    "${COVERAGE_OUTPUT_DIR}/filtered.info"
	    --output-directory "${COVERAGE_OUTPUT_DIR}/html"
	    --title "MUD Coverage Report"
	    --legend
	    --show-details
	COMMENT "Generating HTML coverage report"
	VERBATIM
    )
endif()

# ------------------------------------------------------------------------------
# Custom Target: coverage (Main Entry Point)
#   - Single command that runs the entire coverage workflow
#   - Sequence:
#     - Clean previous data
#     - Capture baseline
#     - Run tests via CTest
#     - Capture test coverage
#     - Combine baseline and test data
#     - Filter out unwanted paths
#     - Generate HTML
# ------------------------------------------------------------------------------
if(MUD_COVERAGE_HTML_ENABLED)
    add_custom_target(coverage
	COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}" --target coverage-clean
	COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}" --target coverage-baseline
	COMMAND ${CMAKE_CTEST_COMMAND} --test-dir "${CMAKE_BINARY_DIR}" --output-on-failure
	COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}" --target coverage-capture
	COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}" --target coverage-combine
	COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}" --target coverage-filter
	COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}" --target coverage-html
	COMMENT "Running full coverage analysis... "
	VERBATIM
    )
endif()

# ------------------------------------------------------------------------------
# Alternative: Minimal Target Without lcov
#   - Provides a `coverage` target even when lcov isn't available
# ------------------------------------------------------------------------------
if(NOT MUD_COVERAGE_HTML_ENABLED)
    add_custom_target(coverage
	COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
	COMMAND ${CMAKE_COMMAND} -E echo "Tests complete. Use gcov manually to analyze .gcda files "
	COMMAND ${CMAKE_COMMAND} -E echo "For HTML reports, install lcov and genhtml."
	COMMENT "Running tests with coverage (no HTML report)"
	VERBATIM
    )
endif()

# ------------------------------------------------------------------------------
# Status Messages:
#   - Reports configuration during CMake run
# ------------------------------------------------------------------------------
message(STATUS "Coverage enabled")
message(STATUS "   gcov: ${GCOV_PATH}")
message(STATUS "   lcov: ${LCOV_PATH}")
message(STATUS "   genhtml: ${GENHTML_PATH}")
message(STATUS "   HTML reports: ${MUD_COVERAGE_HTML_ENABLED}")
message(STATUS "   Output Directory: ${COVERAGE_OUTPUT_DIR}")

#!/usr/bin/bash

# run_tests.sh -  Dynamically discover and runa ll test categories
#
# Usage: ./run_tests.sh
# Options:
#   -v, --verbose:	Show detailed output for each test
#   -s, --stop:		Stop on first failure (default: continue all)
#   -l, --list:		List discovered test categories and exit
#   -h, --help:		Show this help message and exit

set -euo pipefail	# Exit on error, undefined vars, pipe failures

# Configuration
readonly TEST_BINARY="./build/debug/tests/run_all_tests"
readonly TEST_SOURCE_DIR="./tests/modules"
readonly TEST_PREFIX="test_"

# Colors for output (disable if not TTY)
if [[ -t 1 ]]; then
	readonly RED='\033[0;31m'
	readonly GREEN='\033[0;32m'
	readonly YELLOW='\033[0;33m'
	readonly BLUE='\033[0;34m'
	readonly MAGENTA='\033[0;35m'
	readonly CYAN='\033[0;36m'
	readonly WHITE='\033[0;37m'
	readonly RESET='\033[0m'
	readonly BOLD='\033[1m'
else
	readonly RED=''
	readonly GREEN=''
	readonly YELLOW=''
	readonly BLUE=''
	readonly MAGENTA=''
	readonly CYAN=''
	readonly WHITE=''
	readonly RESET=''
	readonly BOLD=''
fi

# Counters for summary
total=0
passed=0
failed=0
skipped=0

# Flags
verbose=false
stop_on_failure=false
list_only=false
run_all=true

#-------------------------------------------------------------------------------
# Functions
#-------------------------------------------------------------------------------

log_info()		{ echo -e "${BLUE}[INFO]${RESET} $*"; }
log_success()		{ echo -e "${GREEN}[SUCCESS]${RESET} $*"; }
log_warn()		{ echo -e "${YELLOW}[WARN]${RESET} $*"; }
log_fail()		{ echo -e "${RED}[FAIL]${RESET} $*"; }

extract_category() {
	local_file="$1"
	local name="${file##*/}"	# basename
	name="${name%.c}"		# Remove .c extension
	echo "${name#${TEST_PREFIX}}"	# Remove test_prefix
}

discover_tests() {
	[[ -d "$TEST_SOURCE_DIR" ]] || return 1

	find "$TEST_SOURCE_DIR" -maxdepth 2 -name "${TEST_PREFIX}*.c" -type f -print0 2>/dev/null | \
		sort -z | \
		while IFS= read -r -d '' file; do
			extract_category "$file"
		done | sort -u
}

run_test() {
	local category="$1"
	if "$TEST_BINARY" -n "$category" >/dev/null 2>&1; then
		log_pass "$category"
		return 0
	else
		log_fail "$category"
		# Show failure output
		"$TEST_BINARY" -n "$category" 2>&1 | sed 's/^/    /'
		return 1
	fi
}

usage() {
	cat << EOF
Usage: ${0##*/} [OPTIONS]

Dynamically discover and run all test categories.

Options:
	-v, --verbose		Show detailed output for each test
	-s, --stop			Stop on first failure (default: continue all)
	-l, --list			List discovered test categories and exit
	-h, --help			Show this help message and exit

Examples:
	${0##*/}			# Run all tests
	${0##*/} -v			# Run with verbose output
	${0##*/} -s			# Stop on first failure
	${0##*/} -l			# List all test categories

EOF
	exit 0
}

#-------------------------------------------------------------------------------
# Test Execution
#-------------------------------------------------------------------------------

run_all_tests() {
	local -a failed_categories=()
	local -a test_categories=()

	log_info "Discovering test categories..."
	mapfile -t test_categories < <(discover_tests)

	if [[ ${#test_categories[@]} -eq 0 ]]; then
		log_warn "No test categories found in $TEST_SOURCE_DIR"
		return 1
	fi

	log_info "Found ${#test_categories[@]} test category(ies): ${test_categories[*]}"
	echo

	for category in "${test_categories[@]}"; do
		((total++))

		if run_single_test "$category"; then
			((passed++))
		else
			((failed++))
			failed_categories+=($category)

			if [[ "$stop_on_failure" == true ]]; then
				log_warn "Stopping on first failure"
				break
			fi
		fi
		echo
	done

	# Print summary
	print_summary "${failed_categories[@]}"
}

print_summary() {
	local -a failed=("$@")

	echo
	echo -e "${BOLD}=======================================================================${RESET}"
	echo -e "${BOLD}                           Test Summary${RESET}"
	echo -e "${BOLD}=======================================================================${RESET}"
	echo -e "Total:		$total"
	echo -e "${GREEN}Passed:	$passed${RESET}"
	if [[ $failed -gt 0 ]]; then
		echo -e "${RED}Failed:	$failed${RESET}"
	fi
	if [[ $skipped -gt 0 ]]; then
		echo -e "${YELLOW}Skipped:	$skipped${RESET}"
	fi
	echo -e "${BOLD}=======================================================================${RESET}"

	if [[ ${#failed[@]} -gt 0 ]]; then
		echo
		log_fail "Failed test categories:"
		for cat in "${failed[@]}"; do
			echo -e "    ${RED}* $cat${RESET}"
		done
		echo
		return 1
	elif [[ $total -eq 0 ]]; then
		log_warn "No tests executed"
		return 1
	else
		log_success "All tests passed!"
		return 0
	fi
}

#-------------------------------------------------------------------------------
# Main Entry Point
#-------------------------------------------------------------------------------

main() {
	# Parse command-line arguments
	while [[ $# -gt 0 ]]; do
		case "$1" in
			-v|--verbose)
				verbose=true
				shift
				;;
			-s|--stop)
				stop_on_failure=true
				shift
				;;
			-l|--list)
				list_only=true
				shift
				;;
			-h|--help)
				usage
				;;
			-ra|--run-all)
				run_all=true
				shift
				;;
			*)
				log_fail "Unknown option: $1"
				echo "Use -h/--help for usage information"
				exit 1
				;;
		esac
	done

	# Check if binary exists and is executable
	if [[ ! -x "$TEST_BINARY" ]]; then
		log_fail "Test binary not found or not executable: $TEST_BINARY"
		log_info "Please build the project first: ninja -C build/debug"
		exit 1
	fi

	if [[ "$run_all" == true ]]; then
		log_info "Running all tests..."
		discover_tests | while read -r cat; do
			run_test "$cat"
			local_result=$?
			if [[ $local_result -ne 0 ]]; then
				result=$local_result
				break
			fi
			echo
			((total++))
			((passed++))
			echo
		done
		run_all_tests
		exit $?
	fi

	# List mode: just show categories and exit
	if [[ "$list_only" == true ]]; then
		log_info "Available test categories:"
		discover_tests | while read -r cat; do
			echo "    * ${TEST_PREFIX}${cat}"
		done
		exit 0
	fi

	# Run the tests
	log_info "Starting test suite..."
	echo
	run_all_tests
	local_result=$?
	print_summary

	exit $result
}

# Run main function with all arguments
main "$@"

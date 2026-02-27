# CMake generated Testfile for 
# Source directory: /home/yosh/gitrepos/personalProjectRepositories/MUD/tests
# Build directory: /home/yosh/gitrepos/personalProjectRepositories/MUD/build/debug/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[run_all_tests]=] "/home/yosh/gitrepos/personalProjectRepositories/MUD/build/debug/tests/run_all_tests")
set_tests_properties([=[run_all_tests]=] PROPERTIES  ENVIRONMENT "MUD_TEST_VERBOSE=1;MUD_LOG_LEVEL=debug" LABELS "unit;all" TIMEOUT "600" WORKING_DIRECTORY "/home/yosh/gitrepos/personalProjectRepositories/MUD" _BACKTRACE_TRIPLES "/home/yosh/gitrepos/personalProjectRepositories/MUD/tests/CMakeLists.txt;65;add_test;/home/yosh/gitrepos/personalProjectRepositories/MUD/tests/CMakeLists.txt;0;")
add_test([=[test_vector]=] "/home/yosh/gitrepos/personalProjectRepositories/MUD/build/debug/tests/test_vector")
set_tests_properties([=[test_vector]=] PROPERTIES  LABELS "unit" TIMEOUT "300" _BACKTRACE_TRIPLES "/home/yosh/gitrepos/personalProjectRepositories/MUD/tests/CMakeLists.txt;103;add_test;/home/yosh/gitrepos/personalProjectRepositories/MUD/tests/CMakeLists.txt;0;")
add_test([=[test_buffer]=] "/home/yosh/gitrepos/personalProjectRepositories/MUD/build/debug/tests/test_buffer")
set_tests_properties([=[test_buffer]=] PROPERTIES  LABELS "unit" TIMEOUT "300" _BACKTRACE_TRIPLES "/home/yosh/gitrepos/personalProjectRepositories/MUD/tests/CMakeLists.txt;103;add_test;/home/yosh/gitrepos/personalProjectRepositories/MUD/tests/CMakeLists.txt;0;")
add_test([=[test_str]=] "/home/yosh/gitrepos/personalProjectRepositories/MUD/build/debug/tests/test_str")
set_tests_properties([=[test_str]=] PROPERTIES  LABELS "unit" TIMEOUT "300" _BACKTRACE_TRIPLES "/home/yosh/gitrepos/personalProjectRepositories/MUD/tests/CMakeLists.txt;103;add_test;/home/yosh/gitrepos/personalProjectRepositories/MUD/tests/CMakeLists.txt;0;")
add_test([=[test_hashmap]=] "/home/yosh/gitrepos/personalProjectRepositories/MUD/build/debug/tests/test_hashmap")
set_tests_properties([=[test_hashmap]=] PROPERTIES  LABELS "unit" TIMEOUT "300" _BACKTRACE_TRIPLES "/home/yosh/gitrepos/personalProjectRepositories/MUD/tests/CMakeLists.txt;103;add_test;/home/yosh/gitrepos/personalProjectRepositories/MUD/tests/CMakeLists.txt;0;")

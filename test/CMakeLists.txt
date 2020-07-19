file(GLOB_RECURSE PIDAN_TEST_SRCS "${PROJECT_SOURCE_DIR}/test/*test.cpp")

add_custom_target(build-tests COMMAND ${CMAKE_CTEST_COMMAND} --show-only)

foreach(pidan_test_src ${PIDAN_TEST_SRCS})
    get_filename_component(pidan_test_filename ${pidan_test_src} NAME)
    string(REPLACE ".cpp" "" pidan_test_name ${pidan_test_filename})

    add_executable(${pidan_test_name} EXCLUDE_FROM_ALL ${pidan_test_src})
    add_dependencies(build-tests ${pidan_test_name})

    target_link_libraries(${pidan_test_name} pidan_shared GTest::GTest GTest::Main ${TBB_LIBRARIES})
    set_target_properties(${pidan_test_name}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test"
        COMMAND ${pidan_test_name}
    )
    add_test(${pidan_test_name} ${CMAKE_BINARY_DIR}/test/${pidan_test_name})
endforeach(pidan_test_src ${PIDAN_TEST_SRCS})
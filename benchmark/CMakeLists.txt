file(GLOB_RECURSE PIDAN_BENCHMARK_SRCS "${PROJECT_SOURCE_DIR}/benchmark/*_benchmark.cpp")

add_custom_target(build-bench)

foreach(pidan_benchmark_src ${PIDAN_BENCHMARK_SRCS})
    get_filename_component(filename ${pidan_benchmark_src} NAME)
    string(REPLACE ".cpp" "" benchmark_name ${filename})

    add_executable(${benchmark_name} EXCLUDE_FROM_ALL ${pidan_benchmark_src})
    add_dependencies(build-bench ${benchmark_name})

    target_link_libraries(${benchmark_name} pidan_shared Threads::Threads ${GBENCHMARK_STATIC_LIB} ${TBB_LIBRARIES})
    set_target_properties(${benchmark_name}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/benchmark"
        COMMAND ${benchmark_name}
    )
endforeach(pidan_benchmark_src ${PIDAN_BENCHMARK_SRCS})


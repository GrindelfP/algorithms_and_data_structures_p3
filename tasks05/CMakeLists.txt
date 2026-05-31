cmake_minimum_required(VERSION 3.16)
project(roofline_bench CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -O2 build (baseline)
add_executable(bench      src/bench_edge_cost.cpp)
add_executable(bench_membw src/bench_membw.cpp)

target_compile_options(bench       PRIVATE -O2)
target_compile_options(bench_membw PRIVATE -O2)

# -O3 build for comparison
add_executable(bench_O3 src/bench_edge_cost.cpp)
target_compile_options(bench_O3 PRIVATE -O3)


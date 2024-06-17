cmake_minimum_required(VERSION 3.25)

find_package(benchmark CONFIG REQUIRED)
find_package(flecs CONFIG REQUIRED)

file(GLOB_RECURSE bench_sources "*.cpp" "*.h")
add_executable(benches ${bench_sources})
target_link_libraries(benches PRIVATE nidavellir flecs::flecs benchmark::benchmark benchmark::benchmark_main)
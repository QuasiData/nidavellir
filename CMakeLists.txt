cmake_minimum_required(VERSION 3.25)

project(nidavellir VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_SYSTEM_LIBRARY_PATH "$ENV{ProgramFiles}/LLVM/lib")

find_package(unordered_dense CONFIG REQUIRED)

set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")

add_library(nidavellir_compiler_flags INTERFACE)

if (MSVC)
    target_compile_options(nidavellir_compiler_flags INTERFACE "/std:c++latest")
endif ()

target_compile_features(nidavellir_compiler_flags INTERFACE cxx_std_23)

target_compile_options(nidavellir_compiler_flags INTERFACE
        "$<${gcc_like_cxx}:$<BUILD_INTERFACE:-Wall;-Wextra>>"
        "$<${msvc_cxx}:$<BUILD_INTERFACE:-W4>>"
)

file(GLOB_RECURSE sources "src/*.cpp" "src/*.h")
add_library(nidavellir ${sources})
target_link_libraries(nidavellir PUBLIC nidavellir_compiler_flags unordered_dense::unordered_dense)
target_include_directories(nidavellir PUBLIC "src/")

find_package(benchmark CONFIG REQUIRED)
find_package(flecs CONFIG REQUIRED)

file(GLOB_RECURSE bench_sources "benches/*.cpp" "benches/*.h")
add_executable(benches ${bench_sources})
target_link_libraries(benches PRIVATE nidavellir flecs::flecs benchmark::benchmark benchmark::benchmark_main)

enable_testing()

file(GLOB_RECURSE test_sources "tests/*.cpp" "tests/*.h")
add_executable(tests ${test_sources})

if (WIN32)
find_package(gtest CONFIG REQUIRED)
target_link_libraries(tests PRIVATE nidavellir GTest::gtest_main)
else()
find_package(PkgConfig)
pkg_search_module(GTEST REQUIRED gtest_main)
target_link_libraries(tests PRIVATE nidavellir ${GTEST_LDFLAGS})
target_compile_options(tests PRIVATE ${GTEST_CFLAGS})
endif()

include(GoogleTest)
gtest_discover_tests(tests)
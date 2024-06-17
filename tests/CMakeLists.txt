cmake_minimum_required(VERSION 3.25)

file(GLOB_RECURSE test_sources "*.cpp" "*.h")
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
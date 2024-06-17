cmake_minimum_required(VERSION 3.25)

file(GLOB_RECURSE sources "*.cpp" "*.h")
add_library(nidavellir ${sources})
target_link_libraries(nidavellir PUBLIC nidavellir_compiler_flags unordered_dense::unordered_dense)
target_include_directories(nidavellir PUBLIC ".")
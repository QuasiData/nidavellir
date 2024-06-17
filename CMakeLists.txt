cmake_minimum_required(VERSION 3.25)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
  CACHE STRING "Vcpkg toolchain file")

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

add_subdirectory(src)

file(GLOB_RECURSE sources "testbed.cpp")
add_executable(testbed ${sources})
target_link_libraries(testbed PUBLIC nidavellir)

add_subdirectory(benches)

enable_testing()

add_subdirectory(tests)
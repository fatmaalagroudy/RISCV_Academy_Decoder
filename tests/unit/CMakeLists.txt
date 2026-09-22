cmake_minimum_required(VERSION 3.16)
project(decoder_unit_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT MSVC)
  add_compile_options(-Wall -Wextra -Werror -Wpedantic)
endif()

set(DECODER_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)

if(EXISTS ${DECODER_ROOT}/third_party/googletest/CMakeLists.txt)
  add_subdirectory(${DECODER_ROOT}/third_party/googletest googletest)
else()
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
  )
  FetchContent_MakeAvailable(googletest)
endif()

add_executable(unit_tests
  test_elf_reader.cpp
  test_decoder_rv32i.cpp
  test_decoder_rv64i.cpp
  test_decoder_compressed.cpp
  test_disassembler.cpp
  ${DECODER_ROOT}/src/elf/elf_reader.cpp
  ${DECODER_ROOT}/src/decode/decoder.cpp
  ${DECODER_ROOT}/src/disasm/disassembler.cpp
)

target_include_directories(unit_tests PRIVATE ${DECODER_ROOT} ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(unit_tests PRIVATE
  TEST_INPUTS_DIR="${DECODER_ROOT}/tests/lit/Inputs")
target_link_libraries(unit_tests PRIVATE GTest::gtest_main)

set_target_properties(unit_tests PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build
  RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/build
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/build
)

cmake_minimum_required(VERSION 3.12)
project(hashtable VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  set(CMAKE_CUDA_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif(CCACHE_PROGRAM)

set(CMAKE_C_CLANG_TIDY clang-tidy)
set(CMAKE_CXX_CLANG_TIDY clang-tidy)

find_package(Catch2 3 REQUIRED)
if(CMAKE_BUILD_TYPE MATCHES "Debug" AND Catch2_FOUND)
  enable_testing()

  add_executable(cpp_test hash-test.cpp)
  target_link_libraries(cpp_test PRIVATE Catch2::Catch2WithMain)
  target_include_directories(cpp_test PRIVATE ./)
  add_test(NAME run_cpp_test COMMAND cpp_test)

  add_executable(ht_test ht-hash.c ht-test.c)
  target_include_directories(ht_test PRIVATE ./)
  add_test(NAME run_ht_test COMMAND ht_test)
endif(CMAKE_BUILD_TYPE MATCHES "Debug" AND Catch2_FOUND)

find_package(Doxygen)
if(Doxygen_FOUND)
	set(DOXYGEN_PREDEFINED "MAKE_DOCS")
	doxygen_add_docs(ht_docs ./ht-hash.h)
endif(Doxygen_FOUND)

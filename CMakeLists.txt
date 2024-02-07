cmake_minimum_required(VERSION 3.17)
set(CMAKE_CXX_STANDARD 14) # Compile as C++14.
set(CMAKE_CXX_STANDARD_REQUIRED ON) # Require C++14 support.
set(BUILD_SHARED_LIBS ON) # We expect external libraries to be linked statically.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(Lars
  DESCRIPTION "Lars"
  LANGUAGES C CXX)

include_directories(include)

add_subdirectory(src)
add_subdirectory(test)

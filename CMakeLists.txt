cmake_minimum_required(VERSION 3.17)
set(CMAKE_CXX_STANDARD 11) # Compile as C++11.
set(CMAKE_CXX_STANDARD_REQUIRED ON) # Require C++11 support.
set(BUILD_SHARED_LIBS ON) # We expect external libraries to be linked statically.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(Lars
  DESCRIPTION "Lars"
  LANGUAGES C CXX)

include_directories(include)

add_subdirectory(src)
add_subdirectory(test)

cmake_minimum_required(VERSION 3.17)
set(CMAKE_CXX_STANDARD 17) # Compile as C++17.
set(CMAKE_CXX_STANDARD_REQUIRED ON) # Require C++17 support.
set(BUILD_SHARED_LIBS ON) # We expect external libraries to be linked statically.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(xxx
  DESCRIPTION "xxx"
  LANGUAGES C CXX)

include_directories(include)

add_subdirectory(src)
add_subdirectory(test)

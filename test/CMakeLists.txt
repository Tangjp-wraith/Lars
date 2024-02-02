add_executable(test test.cc)

target_link_libraries(test 
  lib_name  
  GTest::GTest
  GTest::Main)

# ###### gest
find_package(GTest REQUIRED)
include_directories(${GTest_INCLUDE_DIRS})
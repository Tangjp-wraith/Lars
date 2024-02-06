add_executable(test_io_buffer test_buffer_pool.cc)

target_link_libraries(test_io_buffer
  lars_reactor
  GTest::GTest
  GTest::Main)

# ###### gest
find_package(GTest REQUIRED)
include_directories(${GTest_INCLUDE_DIRS})
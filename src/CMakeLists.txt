add_subdirectory(lars_reactor)
add_executable(main main.cc)
target_link_libraries(main lars_reactor)
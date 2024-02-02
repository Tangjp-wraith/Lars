add_subdirectory(lib_name)
add_executable(main main.cc)
target_link_libraries(main lib_name)
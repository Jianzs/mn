cmake_minimum_required(VERSION 3.9)
project(mn)

set(CMAKE_C_STANDARD 11)

include_directories(include)
aux_source_directory(src source_list)
add_executable(mn ${source_list})
target_link_libraries(mn nl)
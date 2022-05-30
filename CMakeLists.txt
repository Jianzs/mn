cmake_minimum_required(VERSION 3.9)
project(mn)

set(CMAKE_C_STANDARD 11)
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-variable -Wno-unused-parameter")

include_directories(include)

aux_source_directory(src source_list)
add_executable(mn ${source_list})

target_link_libraries(mn nl-3 nl-genl-3)
# target_link_libraries(mn nl)
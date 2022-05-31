cmake_minimum_required(VERSION 3.0.0)
project(mn VERSION 0.0.1)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

include(CTest)
enable_testing()

aux_source_directory(src source_list)
include_directories(include /usr/include/libnl3)
add_executable(mn ${source_list})
target_link_libraries(mn nl-3 nl-genl-3 pthread)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
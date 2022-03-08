cmake_minimum_required(VERSION 3.16)
project(d3fuse C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -no-pie")
set(CMAKE_C_STANDARD 11)

add_executable(d3fuse main.c fs.c fs.h file.c file.h)
target_link_libraries(d3fuse fuse3)


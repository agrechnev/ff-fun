cmake_minimum_required(VERSION 3.0)
project (tut)

#set (CMAKE_CXX_STANDARD 14)
set (CMAKE_C_STANDARD 11)

add_executable(tutorial01 tutorial01.c)
target_link_libraries (tutorial01 ${FF_LIBS})

cmake_minimum_required(VERSION 3.1)
project (fun1)

set (CMAKE_CXX_STANDARD 14)
set (CMAKE_C_STANDARD 11)

set(SRCS
    fun1.c
)

add_executable(${PROJECT_NAME} ${SRCS})
target_link_libraries (${PROJECT_NAME} ${FF_LIBS})

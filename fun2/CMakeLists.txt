cmake_minimum_required(VERSION 3.0)
project (fun2)

#set (CMAKE_CXX_STANDARD 14)
set (CMAKE_C_STANDARD 11)

set(SRCS
    fun2.c
)

add_executable(${PROJECT_NAME} ${SRCS})
target_link_libraries (${PROJECT_NAME} ${FF_LIBS})

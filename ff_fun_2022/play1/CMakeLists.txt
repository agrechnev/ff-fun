cmake_minimum_required(VERSION 3.1)
project (play1)

set(SRCS
        play1.cpp
)

add_executable(${PROJECT_NAME} ${SRCS})
target_link_libraries (${PROJECT_NAME} ${FF_LIBS} ${OpenCV_LIBS})

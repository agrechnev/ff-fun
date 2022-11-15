cmake_minimum_required(VERSION 3.1)
project (funcv2)

set (CMAKE_CXX_STANDARD 14)
set (CMAKE_C_STANDARD 11)


find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

set(SRCS
    main.cpp
    VEncoder.cpp VEncoder.h
    VDecoder.cpp VDecoder.h
    fatal.h
)

add_executable(${PROJECT_NAME} ${SRCS})
target_link_libraries (${PROJECT_NAME} ${FF_LIBS} ${OpenCV_LIBS})

cmake_minimum_required(VERSION 3.0)
project (funcv2)

set (CMAKE_CXX_STANDARD 14)
#set (CMAKE_C_STANDARD 11)


if(WIN32)
 set(OpenCV_DIR d:/opencv)
else()
 set(OpenCV_DIR  ~/opencv3/share/OpenCV)
endif()


find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

set(SRCS
    main.cpp
    VCoder.cpp VCoder.h
    fatal.h
)

add_executable(${PROJECT_NAME} ${SRCS})
target_link_libraries (${PROJECT_NAME} ${FF_LIBS} ${OpenCV_LIBS})

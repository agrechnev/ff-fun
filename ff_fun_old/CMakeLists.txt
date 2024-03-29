cmake_minimum_required(VERSION 3.1)
project (ff-fun-old)

set (CMAKE_CXX_STANDARD 14)
set (CMAKE_C_STANDARD 11)

# OpenCV
find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

# ffmpeg
find_package(PkgConfig REQUIRED)
pkg_search_module(AVFORMAT required libavformat)
pkg_search_module(AVCODEC required libavcodec)
pkg_search_module(AVUTIL required libavutil)
pkg_search_module(SWSCALE required libswscale)

find_package(Threads)

include_directories(${AVFORMAT_INCLUDE_DIRS} ${AVCODEC_INCLUDE_DIRS} ${AVUTIL_INCLUDE_DIRS}  ${SWSCALE_INCLUDE_DIRS})

set(FF_LIBS
    ${AVFORMAT_LIBRARIES}  ${AVCODEC_LIBRARIES}  ${AVUTIL_LIBRARIES}  ${SWSCALE_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} 
)

# libm and libdl only for unix
if(UNIX)
 set(FF_LIBS ${FF_LIBS} m dl)
endif()

MESSAGE(STATUS "FF_LIBS = ${FF_LIBS}")

add_subdirectory(play1)
add_subdirectory(play2)

# OLD examples
add_subdirectory(fun1)
add_subdirectory(fun2)
add_subdirectory(funcv)
add_subdirectory(funcv2)

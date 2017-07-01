cmake_minimum_required(VERSION 3.0)
project (ff-fun)

#set (CMAKE_CXX_STANDARD 14)
set (CMAKE_C_STANDARD 11)

IF(NOT CMAKE_BUILD_TYPE )
   SET( CMAKE_BUILD_TYPE "Debug" )
ENDIF()

# Find cmake libraries
# avformat would be enough for linux, need all 3 in mingw for some reason
find_package(PkgConfig REQUIRED)
pkg_search_module(AVFORMAT required libavformat)
pkg_search_module(AVCODEC required libavcodec)
pkg_search_module(AVUTIL required libavutil)

find_package(Threads)

include_directories(${AVFORMAT_INCLUDE_DIRS} ${AVCODEC_INCLUDE_DIRS} ${AVUTIL_INCLUDE_DIRS})

set(FF_LIBS
    ${AVFORMAT_LIBRARIES}  ${AVCODEC_LIBRARIES}  ${AVUTIL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} swscale
)

# libm and libdl only for unix
if(UNIX)
 set(FF_LIBS ${FF_LIBS} m dl)
endif()

MESSAGE(STATUS "FF_LIBS = ${FF_LIBS}")

add_subdirectory(tut)
add_subdirectory(fun1)
add_subdirectory(fun2)
add_subdirectory(funcv)
add_subdirectory(funcv2)

cmake_minimum_required(VERSION 3.0)
project (ff-fun)

#set (CMAKE_CXX_STANDARD 14)
set (CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_search_module(FFMPEG required libavformat)

find_package(Threads)

include_directories(${FFMPEG_INCLUDE_DIRS})

set(FF_LIBS
    ${FFMPEG_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} swscale m dl
)

add_subdirectory(tut)
add_subdirectory(fun)

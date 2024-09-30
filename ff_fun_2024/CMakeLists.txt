cmake_minimum_required(VERSION 3.10)
project(ff-fun-2024)
set(CMAKE_CXX_STANDARD 20)


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

add_subdirectory(play1)

add_executable(fun1 fun1.cpp)
target_link_libraries (fun1 ${FF_LIBS} ${OpenCV_LIBS})

add_executable(fun2_encode fun2_encode.cpp)
target_link_libraries (fun2_encode ${FF_LIBS} ${OpenCV_LIBS})

add_executable(fun3_encode fun3_encode.cpp)
target_link_libraries (fun3_encode ${FF_LIBS} ${OpenCV_LIBS})

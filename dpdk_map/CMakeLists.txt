cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 11)

#aux_source_directory(. SOURCE_FILES )
set(CMAKE_CXX_FLAGS "-msse4.2")
set(SOURCE_FILES dpdk_map.cpp)
include_directories( /home/qjt/my_dpdk/dpdk_map)

# 生成目标库   SHARED动态库  STATIC 静态库
add_library(dpdk_map ${SOURCE_FILES} )
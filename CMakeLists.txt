project(ShaderToy)
cmake_minimum_required(VERSION 2.8)

set( CMAKE_CXX_FLAGS "-std=c++0x -Wall ${CMAKE_CXX_FLAGS}" )
if(_CLANG_)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

# Find Pangolin (https://github.com/stevenlovegrove/Pangolin)
find_package(Pangolin 0.1 REQUIRED)
include_directories(${Pangolin_INCLUDE_DIRS})

add_executable(ShaderToy main.cpp)
target_link_libraries(ShaderToy ${Pangolin_LIBRARIES})

cmake_minimum_required(VERSION 3.8)
project(AsynchronousDownloader)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp)
add_executable(AsynchronousDownloader ${SOURCE_FILES})
target_link_libraries(AsynchronousDownloader curl)
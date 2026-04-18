cmake_minimum_required(VERSION 4.0)

project(IspPipeline LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(pipeline)
add_subdirectory(app)

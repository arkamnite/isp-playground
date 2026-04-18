add_library(pipeline)

target_sources(pipeline PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/bayer.cpp")
target_include_directories(pipeline PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(pipeline PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/internal")

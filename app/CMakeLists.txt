add_executable(isp_app)

target_sources(isp_app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
target_link_libraries(isp_app PRIVATE pipeline)

add_executable(test)
target_sources(test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/sketchpad.c")

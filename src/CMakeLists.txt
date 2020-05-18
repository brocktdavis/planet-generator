SET(pwd ${CMAKE_CURRENT_LIST_DIR})

SET(src "")
AUX_SOURCE_DIRECTORY(${pwd} src)
add_executable(planets ${src})
message(STATUS "planets added ${src}")

target_link_libraries(planets ${stdgl_libraries})
TARGET_LINK_LIBRARIES(planets ${JPEG_LIBRARIES})
TARGET_LINK_LIBRARIES(planets ${Boost_LIBRARIES})
TARGET_LINK_LIBRARIES(planets quickhull)
TARGET_LINK_LIBRARIES(planets noise)

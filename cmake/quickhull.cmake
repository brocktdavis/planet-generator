INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/lib/quickhull)
AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/lib/quickhull quickhull_src)

ADD_LIBRARY(quickhull STATIC ${quickhull_src})
message("quickhull " ${quickhull_src})

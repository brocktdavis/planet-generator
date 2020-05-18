INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/lib/SimplexNoise/src)
AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/lib/SimplexNoise/src noise_src)

ADD_LIBRARY(noise STATIC ${noise_src})
message("noise " ${noise_src})

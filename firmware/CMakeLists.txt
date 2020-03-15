# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

#include_directories("/home/julian/development/uc/libs/ringbuffer")
#add_subdirectory("/home/julian/development/uc/libs/ringbuffer" "ringbuffer")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(flipdot)

#target_link_libraries(dummy_main_src.c "-Lringbuffer")
#target_link_libraries(${COMPONENT_LIB} PUBLIC ringbuffer)

#add_library(ringbuffer2 STATIC IMPORTED)
#set_property(TARGET ringbuffer2 PROPERTY IMPORTED_LOCATION /home/julian/flipdot/flipdot-brose/firmware/build/ringbuffer/libringbuffer.a)

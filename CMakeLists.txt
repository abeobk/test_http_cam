cmake_minimum_required(VERSION 3.2)

project(test_http_cam)
find_package( OpenCV REQUIRED )
find_package( Threads REQUIRED )

include_directories( 
    ${OpenCV_INCLUDE_DIRS} 
    /usr/include/abeopilib/
)

add_executable(test_http_cam test_http_cam.cpp )

target_link_libraries(test_http_cam
    ${OpenCV_LIBS} 
    ${CMAKE_THREAD_LIBS_INIT}
    /usr/lib/libabeopilib.so
)

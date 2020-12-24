cmake_minimum_required(VERSION 3.2)
message ("ABEO MSG")
project( test_cv )
find_package( OpenCV REQUIRED )
find_package( Threads REQUIRED )

include_directories( 
    ${OpenCV_INCLUDE_DIRS} 
    /usr/include/abeopilib/
)

add_executable( test_http test_http.cpp )

target_link_libraries( test_http
    ${OpenCV_LIBS} 
    ${CMAKE_THREAD_LIBS_INIT}
    /usr/lib/libabeopilib.so
)

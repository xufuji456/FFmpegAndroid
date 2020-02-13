# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)
set(distribution_DIR ${CMAKE_SOURCE_DIR}/../../../../libs)

add_library(faac
             STATIC
             IMPORTED)
set_target_properties(faac
                       PROPERTIES IMPORTED_LOCATION
                       ../../../../libs/${CMAKE_ANDROID_ARCH_ABI}/libfaac.a)

add_library(rtmp
             STATIC
             IMPORTED)
set_target_properties(rtmp
                       PROPERTIES IMPORTED_LOCATION
                       ../../../../libs/${CMAKE_ANDROID_ARCH_ABI}/librtmp.a)

add_library(x264
            STATIC
            IMPORTED)
set_target_properties(x264
                      PROPERTIES IMPORTED_LOCATION
                      ../../../../libs/${CMAKE_ANDROID_ARCH_ABI}/libx264.a)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

include_directories(main/cpp/include)

add_library(live
        SHARED
        src/main/cpp/AudioStream.cpp
        src/main/cpp/VideoStream.cpp
        src/main/cpp/RtmpPusher.cpp)

find_library( log-lib
              log )

target_link_libraries(live x264 faac rtmp
                       -landroid
                       -ljnigraphics
                       ${log-lib} )
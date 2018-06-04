# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
             media-handle

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).

             src/main/cpp/ffmpeg/cmdutils.c
             src/main/cpp/ffmpeg/ffmpeg.c
             src/main/cpp/ffmpeg/ffmpeg_filter.c
             src/main/cpp/ffmpeg/ffmpeg_opt.c
             src/main/cpp/ffmpeg_cmd.c
             src/main/cpp/audio_player.c
             src/main/cpp/openSL_audio_player.c
             src/main/cpp/video_player.c
             src/main/cpp/ffmpeg_pusher.cpp
             src/main/cpp/AVpacket_queue.c
             src/main/cpp/media_player.c
             src/main/cpp/video_filter.c)

add_library( ffmpeg
             SHARED
             IMPORTED )
set_target_properties( ffmpeg
                       PROPERTIES IMPORTED_LOCATION
                       ../../../../libs/armeabi-v7a/libffmpeg.so )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

include_directories(src/main/cpp)
include_directories(src/main/cpp/include)

find_library( # Sets the name of the path variable.
              log-lib
              log )

target_link_libraries( # Specifies the target library.
                       media-handle
                       ffmpeg
                       -landroid #native_window
                       -ljnigraphics #bitmap
                       -lOpenSLES #openSLES
                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib} )
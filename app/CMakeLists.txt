# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
             audio-handle

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             src/main/cpp/ffmpeg_cmd.c
             src/main/cpp/cmdutils.c
             src/main/cpp/ffmpeg.c
             src/main/cpp/ffmpeg_filter.c
             src/main/cpp/ffmpeg_opt.c)

add_library( ffmpeg
             SHARED
             IMPORTED )
set_target_properties( ffmpeg
                       PROPERTIES IMPORTED_LOCATION
                       ../../../../libs/armeabi-v7a/libffmpeg.so )

include_directories(src/main/cpp)
include_directories(src/main/cpp/include)

find_library( # Sets the name of the path variable.
              log-lib
              log )

target_link_libraries( # Specifies the target library.
                       audio-handle
                       ffmpeg
                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib} )
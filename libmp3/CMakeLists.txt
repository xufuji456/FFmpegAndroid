# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
             mp3-lite

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             src/main/cpp/audio_lame.c
        )

add_library( mp3lame
             SHARED
             IMPORTED )
set_target_properties( mp3lame
            PROPERTIES IMPORTED_LOCATION
            ../../../../libs/${CMAKE_ANDROID_ARCH_ABI}/libmp3lame.so )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

include_directories(src/main/cpp)

find_library( # Sets the name of the path variable.
              log-lib
              log )

target_link_libraries( # Specifies the target library.
                       mp3-lite
                       mp3lame
                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib} )
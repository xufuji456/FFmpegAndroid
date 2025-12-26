# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

set(SRC_FFMPEG
        ffmpeg/cmdutils.c
        ffmpeg/ffmpeg.c
        ffmpeg/ffmpeg_demux.c
        ffmpeg/ffmpeg_filter.c
        ffmpeg/ffmpeg_hw.c
        ffmpeg/ffmpeg_mux.c
        ffmpeg/ffmpeg_mux_init.c
        ffmpeg/ffmpeg_opt.c
        ffmpeg/ffprobe.c
        ffmpeg/objpool.c
        ffmpeg/opt_common.c
        ffmpeg/sync_queue.c
        ffmpeg/thread_queue.c)

set(SRC_VISUALIZER
        visualizer/fft.cpp
        visualizer/fixed_fft.cpp
        visualizer/frank_visualizer.cpp
        visualizer/frank_visualizer_jni.cpp
        visualizer/window.cpp)

set(SRC_METADATA
        metadata/media_retriever.cpp
        metadata/media_retriever_jni.cpp
        metadata/metadata_util.c
        metadata/ffmpeg_media_retriever.c)

set(SRC_FFPLAYER ffplayer)

add_library( # Sets the name of the library.
             media-handle

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
        ${SRC_FFMPEG}
        ${SRC_VISUALIZER}
        ${SRC_METADATA}
        ffmpeg_cmd.c
        video_filter.c
        media_transcode.cpp
        ff_audio_player.cpp
        audio_player_jni.cpp
)

add_library( ffmpeg
             SHARED
             IMPORTED )
set_target_properties( ffmpeg
                       PROPERTIES IMPORTED_LOCATION
                       ../../../../libs/${CMAKE_ANDROID_ARCH_ABI}/libffmpeg.so )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

include_directories(.)
include_directories(include)
include_directories(metadata)
include_directories(ffplayer)

if(${CMAKE_ANDROID_ARCH_ABI} MATCHES "armeabi-v7a")
    include_directories(include/armeabi-v7a/libavcodec)
    include_directories(include/armeabi-v7a/libavutil)
    message("This is armeabi-v7a")
elseif(${CMAKE_ANDROID_ARCH_ABI} MATCHES "arm64-v8a")
    include_directories(include/arm64-v8a/libavcodec)
    include_directories(include/arm64-v8a/libavutil)
    message("This is arm64-v8a")
endif()

find_library( # Sets the name of the path variable.
              log-lib
              log )

target_link_options(media-handle PRIVATE "-Wl,-z,max-page-size=16384")

target_link_libraries( # Specifies the target library.
                       media-handle
                       ffmpeg
                       -landroid #native_window
                       -ljnigraphics #bitmap
                       -lOpenSLES #openSLES
                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib} )
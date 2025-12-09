cmake_minimum_required(VERSION 3.10.2)

project(decode)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wno-deprecated")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG ")

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(EXTRA_DIR "${ROOT_DIR}/../extra")
set(COMMON_DIR "${ROOT_DIR}/../common")
set(EXTRA_COMMON_DIR "${COMMON_DIR}")
set(EXTRA_FFMPEG_ROOT "${EXTRA_DIR}/ffmpeg")

set(SRC_LIST
        common/nal_convert.c
        common/MixedBuffer.cpp
        common/h26x_parser.c
        VideoDecoder.cpp
        VideoDecoderFactory.cpp
        FFmpegVideoDecoder.cpp
        FFmpegAudioDecoder.cpp
)

if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(TARGET_PLATFORM android)
    set(CMAKE_SYSTEM_VERSION 21)
    set(EXTRA_FFMPEG_DIR
            "${EXTRA_DIR}/ffmpeg/${TARGET_PLATFORM}/${CMAKE_ANDROID_ARCH_ABI}")
    set(CMAKE_ANDROID_NDK $ENV{ANDROID_NDK})
#    set(SRC_LIST ${SRC_LIST}
#            android/MediacodecDecoder.cpp)
elseif (CMAKE_SYSTEM_NAME STREQUAL "OHOS")
    add_definitions(-D__HARMONY__)
    set(TARGET_PLATFORM harmony)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-command-line-argument")
    set(EXTRA_FFMPEG_DIR "${EXTRA_DIR}/ffmpeg/${TARGET_PLATFORM}/${OHOS_ARCH}")
else ()
    message(FATAL_ERROR "Don't support ${CMAKE_SYSTEM_NAME}!")
endif ()

include_directories(
        "${EXTRA_FFMPEG_ROOT}/include"
        "${EXTRA_COMMON_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/..")

link_directories("${EXTRA_FFMPEG_DIR}")

add_library(decode SHARED ${SRC_LIST})

if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    target_link_libraries(
            decode
            mediandk
            android
            common
            ffmpeg
    )
elseif (CMAKE_SYSTEM_NAME STREQUAL "OHOS")
    target_link_libraries(
            decode
            ffmpeg
            common
            native_media_vdec
            native_media_core
            native_media_codecbase
            native_window
    )
endif ()

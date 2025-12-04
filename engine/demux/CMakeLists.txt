cmake_minimum_required(VERSION 3.10.2)

project(demux)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wno-deprecated")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG ")

set(EXTRA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../extra")
set(COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../common")
set(EXTRA_FFMPEG_ROOT "${EXTRA_DIR}/ffmpeg")

if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(TARGET_PLATFORM android)
    set(CMAKE_SYSTEM_VERSION 21)
    set(CMAKE_ANDROID_NDK $ENV{ANDROID_NDK})
    set(EXTRA_FFMPEG_DIR
            "${EXTRA_DIR}/ffmpeg/${TARGET_PLATFORM}/${CMAKE_ANDROID_ARCH_ABI}")
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
        "${COMMON_DIR}/include"
        ${CMAKE_CURRENT_SOURCE_DIR})

link_directories("${EXTRA_FFMPEG_DIR}")

set(SRC_LIST
        FFmpegUtil.cpp
        NextExtractor.cpp)

add_library(demux SHARED ${SRC_LIST})

if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    find_library(log-lib log)
    target_link_libraries(
            demux
            ${log-lib}
            ffmpeg
            common)
elseif (CMAKE_SYSTEM_NAME STREQUAL "OHOS")
    find_library(log-lib hilog_ndk.z)
    target_link_libraries(
            demux
            ${log-lib}
            ffmpeg
            common)
else ()
    message(FATAL_ERROR "Don't support ${CMAKE_SYSTEM_NAME}!")
endif ()

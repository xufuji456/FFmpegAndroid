cmake_minimum_required(VERSION 3.10.2)

project(common)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wno-deprecated")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG ")

set(SRC_LIST src/NextDictionary.cpp src/NextLog.cpp)

if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(CMAKE_ANDROID_NDK $ENV{ANDROID_NDK})
    set(CMAKE_SYSTEM_VERSION 21)
    aux_source_directory(src/android SRC_LIST_ANDROID)
    set(SRC_LIST
        ${SRC_LIST}
        ${SRC_LIST_ANDROID})
elseif (CMAKE_SYSTEM_NAME STREQUAL "OHOS")
    add_definitions(-D__HARMONY__)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-command-line-argument")
else ()
    message(FATAL_ERROR "Don't support ${CMAKE_SYSTEM_NAME}!")
endif ()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

add_library(common SHARED ${SRC_LIST})

if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    find_library(log-lib log)
elseif (CMAKE_SYSTEM_NAME STREQUAL "OHOS")
    find_library(log-lib hilog_ndk.z)
else ()
    message(FATAL_ERROR "Don't support ${CMAKE_SYSTEM_NAME}!")
endif ()

target_link_libraries(common ${log-lib})

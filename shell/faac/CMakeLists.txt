cmake_minimum_required(VERSION 3.4.1)

project("faac")

# the path of faac source code
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/libfaac FAAC_SRC)

add_library(faac
        SHARED
        ${FAAC_SRC})

target_link_libraries(faac)
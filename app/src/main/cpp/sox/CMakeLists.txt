
set(PROJECT_NAME sox)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(CMAKE_COMPILER_IS_GNUCC)
    execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion
            OUTPUT_VARIABLE ver)
    string(REGEX REPLACE "([0-9]+)\\.([0-9]+).*" "\\1" major "${ver}")
    string(REGEX REPLACE "([0-9]+)\\.([0-9]+).*" "\\2" minor "${ver}")
    math(EXPR ver "100 * ${major} + ${minor}")
    if(${ver} LESS 403)
        add_definitions(-Wconversion)
    else(${ver} LESS 403)
        add_definitions(-Wtraditional-conversion)
    endif(${ver} LESS 403)
endif(CMAKE_COMPILER_IS_GNUCC)

if (NOT EXTERNAL_GSM)
    set(optional_libs ${optional_libs} gsm)
endif (NOT EXTERNAL_GSM)

set(effects_srcs
        delay.c
        downsample.c
        echo.c
        echos.c
        fft4g.c
        input.c
        output.c
        reverb.c
        speed.c
        tempo.c
        upsample.c
        vad.c
        vol.c
        )
set(formats_srcs
        adpcm.c
        g711.c
        ima_rw.c
        raw.c
        wav.c
        )

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/bit-rot)

add_library(${PROJECT_NAME} STATIC
        ${effects_srcs}
        effects.c
        effects_i.c
        effects_i_dsp.c
        ${formats_srcs}
        formats_i.c
        formats.c
        getopt.c
        libsox_i.c
        libsox.c
        util.c
        xmalloc.c
        )

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

find_program(LN ln)
if (LN)
    add_custom_target(rec ALL ${LN} -sf sox rec DEPENDS sox)
    add_custom_target(play ALL ${LN} -sf sox play DEPENDS sox)
    add_custom_target(soxi ALL ${LN} -sf sox soxi DEPENDS sox)
endif (LN)
find_program(CTAGS NAMES exuberant-ctags ctags)
add_custom_target(tags ${CTAGS} --recurse --extra=fq .)
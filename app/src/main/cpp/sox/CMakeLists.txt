
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
        bend.c
        biquad.c
        biquads.c
        chorus.c
        compand.c
        compandt.c
        contrast.c
        dcshift.c
        delay.c
        dft_filter.c
        dither.c
        divide.c
        downsample.c
        earwax.c
        echo.c
        echos.c
        fade.c
        fft4g.c
        fir.c
        firfit.c
        flanger.c
        gain.c
        hilbert.c
        input.c
        loudness.c
        mcompand.c
        noiseprof.c
        noisered.c
        output.c
        overdrive.c
        pad.c
        phaser.c
        rate.c
        remix.c
        repeat.c
        reverb.c
        reverse.c
        silence.c
        sinc.c
        skeleff.c
        speed.c
        splice.c
        stat.c
        stats.c
        stretch.c
        swap.c
        synth.c
        tempo.c
        tremolo.c
        trim.c
        upsample.c
        vad.c
        vol.c
        )
set(formats_srcs
        adpcm.c
        adpcms.c
        al-fmt.c
        coreaudio.c
        f4-fmt.c
        f8-fmt.c
        g711.c
        g721.c
        g723_24.c
        g723_40.c
        g72x.c
        ima-fmt.c
        ima_rw.c
        la-fmt.c
        lu-fmt.c
        raw.c
        raw-fmt.c
        s1-fmt.c
        s2-fmt.c
        s3-fmt.c
        s4-fmt.c
        sox-fmt.c
        u1-fmt.c
        u2-fmt.c
        u3-fmt.c
        u4-fmt.c
        ul-fmt.c
        voc.c
        vox.c
        vox-fmt.c
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
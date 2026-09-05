# Vendored SEP (Source Extractor as a library), github.com/PJ-Watson/sep-pjw, LGPL-3.0.
# See PROVENANCE.txt for the exact commit. Built static and warning-suppressed so the project's
# -Werror does not apply to third-party C. aperture.i / overlap.h / extract.h are #included, not
# compiled directly.
add_library(sep STATIC
    src/analyse.c
    src/aperture.c
    src/background.c
    src/convolve.c
    src/deblend.c
    src/extract.c
    src/lutz.c
    src/util.c
)

target_include_directories(sep PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)

set_target_properties(sep PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS ON # SEP uses M_PI, which glibc gates behind GNU extensions (gnu11, not strict c11)
    POSITION_INDEPENDENT_CODE ON
)

target_compile_options(sep PRIVATE -w)

set(GM_ENABLE_TESTS OFF CACHE BOOL "Disable GameMath tests" FORCE)

FetchContent_Declare(
    gamemath
    GIT_REPOSITORY https://github.com/TheSuperHackers/GameMath.git
    GIT_TAG        59f7ccd494f7e7c916a784ac26ef266f9f09d78d
)

FetchContent_MakeAvailable(gamemath)

# GameMath uses SSE/SSE2 intrinsics for its rounding functions. 32-bit MinGW-w64 GCC
# does not enable SSE by default, which fails with "inlining failed ... target specific option mismatch".
if(MINGW)
    target_compile_options(gamemath PRIVATE -msse2)
endif()

# Ensure GameMath includes are available to ALL targets
# to prevent one-definition-rule violations and ensure USE_DETERMINISTIC_MATH activates consistently.
include_directories(${gamemath_SOURCE_DIR}/include)

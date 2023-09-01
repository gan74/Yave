cmake_minimum_required(VERSION 3.18)
project(y)


set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)

# This causes recursive macros to generate a bunch of warnings, solved by C++20
# set(CMAKE_CXX_EXTENSIONS OFF)


if(MSVC)
    # https://docs.microsoft.com/en-us/cpp/build/reference/zc-conformance?view=msvc-160
    # https://docs.microsoft.com/en-us/cpp/build/reference/zm-specify-precompiled-header-memory-allocation-limit?view=msvc-160

    # Disabled warnings:
    # C4702 unreachable code
    # C4127 conditional expression is constant, does wierd things w/ constexpr
    set(Y_COMPILE_OPTIONS /permissive- /fp:fast /W4 /wd4702 /wd4127)

    set(Y_COMPILE_OPTIONS ${Y_COMPILE_OPTIONS} -D_CRT_SECURE_NO_DEPRECATE)
    set(Y_COMPILE_OPTIONS ${Y_COMPILE_OPTIONS} /Zc:preprocessor /bigobj)

    # Allow some inlining in debug to make stepping easier
    # string(REPLACE "/Ob0" "/Ob1" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
else()
    # -Wsign-conversion -Wshadow
    set(Y_EXTRA_WARNINGS
            -Wzero-as-null-pointer-constant
            -Wfloat-conversion
            -Woverloaded-virtual
            -Wnon-virtual-dtor
            -Wlogical-op
            -Wredundant-decls
            -Wundef
            -Wno-gnu-zero-variadic-macro-arguments
        )
    set(Y_COMPILE_OPTIONS -pedantic -Wall -Wextra
        ${Y_EXTRA_WARNINGS})

    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -fomit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -Wodr")
endif()



file(GLOB_RECURSE SOURCE_FILES
        "y/*.h"
        "y/*.cpp"
    )

file(GLOB_RECURSE TEST_FILES
        "tests/*.cpp"
    )


add_library(y STATIC ${SOURCE_FILES})

# compile options are exported, this is generally bad, but it makes stuff simpler
target_compile_options(y PUBLIC ${Y_COMPILE_OPTIONS})
target_compile_options(y PUBLIC
    $<$<CONFIG:RELEASE>:-DY_NO_DEBUG>
    $<$<NOT:$<CONFIG:RELEASE>>:-DY_DEBUG>)


option(Y_BUILD_TESTS "Build tests" ON)
if(Y_BUILD_TESTS)
    add_executable(tests ${TEST_FILES} "tests.cpp")
    target_compile_definitions(tests PRIVATE "-DY_BUILD_TESTS")
    target_link_libraries(tests y)
endif()


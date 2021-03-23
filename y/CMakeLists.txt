cmake_minimum_required(VERSION 3.18)
project(y)


set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)

# This causes recursive macros to generate a bunch of warnings, solved by C++20
# set(CMAKE_CXX_EXTENSIONS OFF)


if(MSVC)
    # https://docs.microsoft.com/en-us/cpp/build/reference/zc-conformance?view=msvc-160
    # https://docs.microsoft.com/en-us/cpp/build/reference/zm-specify-precompiled-header-memory-allocation-limit?view=msvc-160
    set(Y_COMPILE_OPTIONS /permissive- /W3 /WX)
    set(Y_COMPILE_OPTIONS ${Y_COMPILE_OPTIONS} -D_CRT_SECURE_NO_DEPRECATE)
    set(Y_COMPILE_OPTIONS ${Y_COMPILE_OPTIONS} /Zc:preprocessor)

    option(Y_USE_ASAN "Enable ASAN" OFF)

    if(Y_USE_ASAN)
        set(Y_COMPILE_OPTIONS ${Y_COMPILE_OPTIONS} /fsanitize=address)
    endif()
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
        )
    set(Y_COMPILE_OPTIONS -pedantic -Wall -Wextra ${Y_EXTRA_WARNINGS})

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


option(Y_FORCE_ASSERTS "Build asserts even in release" OFF)
if((Y_FORCE_ASSERTS) OR NOT (CMAKE_BUILD_TYPE STREQUAL Release))
    target_compile_options(y PUBLIC "-DY_DEBUG")
endif()

if((NOT Y_FORCE_ASSERTS) AND (CMAKE_BUILD_TYPE STREQUAL Release))
    target_compile_options(y PUBLIC "-DY_NO_DEBUG")
endif()


option(Y_BUILD_TESTS "Build tests" ON)
if(Y_BUILD_TESTS)
    #enable_testing()
    add_executable(tests ${TEST_FILES} "tests.cpp")
    target_compile_definitions(tests PRIVATE "-DY_BUILD_TESTS")
    target_link_libraries(tests y)
    #add_test(Test tests)
endif()


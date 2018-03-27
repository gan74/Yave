cmake_minimum_required(VERSION 3.7)
project(y)

include_directories(${y_SOURCE_DIR})

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -fomit-frame-pointer")

set(ULTIMATE_WARNINGS -Wdisabled-optimization -Wlogical-op -Wwrite-strings -Wmissing-include-dirs -Wredundant-decls -Wsign-promo -Wswitch-default -Wundef)
set(EXTRA_WARNINGS -Woverloaded-virtual -Wcast-qual -Wcast-align -Wunreachable-code -Wold-style-cast -Wnon-virtual-dtor -Wodr)

set(Y_COMPITE_OPTIONS -std=c++17 -pedantic -Wall -Wextra ${EXTRA_WARNINGS} ${ULTIMATE_WARNINGS})


file(GLOB_RECURSE SOURCE_FILES
		"y/*.h"
		"y/*.cpp"
	)

file(GLOB_RECURSE TEST_FILES
		"tests/*.cpp"
	)


option(BUILD_TESTS "Build tests" ON)


add_library(y STATIC ${SOURCE_FILES})
target_link_libraries(y pthread)
target_compile_options(y PUBLIC ${Y_COMPITE_OPTIONS})


if(BUILD_TESTS)
	#enable_testing()
	add_executable(tests ${TEST_FILES} "tests.cpp")
	target_compile_definitions(tests PRIVATE "-DY_BUILD_TESTS")
	target_link_libraries(tests y)
	#add_test(Test tests)
endif()

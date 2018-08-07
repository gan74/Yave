cmake_minimum_required(VERSION 3.7)
project(y)

set(ULTIMATE_WARNINGS -Wdisabled-optimization -Wlogical-op -Wredundant-decls -Wsign-promo -Wswitch-default -Wundef)
set(EXTRA_WARNINGS -Woverloaded-virtual -Wcast-qual -Wold-style-cast -Wnon-virtual-dtor -Wodr)
set(Y_COMPILE_OPTIONS -pedantic -Wall -Wextra ${EXTRA_WARNINGS} ${ULTIMATE_WARNINGS})
#set(Y_COMPILE_OPTIONS ${Y_COMPILE_OPTIONS} PARENT_SCOPE)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -fomit-frame-pointer")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)


file(GLOB_RECURSE SOURCE_FILES
		"y/*.h"
		"y/*.cpp"
	)

file(GLOB_RECURSE TEST_FILES
		"tests/*.cpp"
	)

add_library(y STATIC ${SOURCE_FILES})
#target_link_libraries(y pthread)
target_compile_options(y PUBLIC ${Y_COMPILE_OPTIONS})


if(CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
	target_compile_options(y PUBLIC "-DY_DEBUG")
endif()

if(CMAKE_BUILD_TYPE STREQUAL Release)
	target_compile_options(y PUBLIC "-DY_NO_DEBUG")
endif()


option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
	#enable_testing()
	add_executable(tests ${TEST_FILES} "tests.cpp")
	target_compile_definitions(tests PRIVATE "-DY_BUILD_TESTS")
	target_link_libraries(tests y)
	#add_test(Test tests)
endif()

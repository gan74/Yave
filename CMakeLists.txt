cmake_minimum_required(VERSION 3.6)
project(y)

include_directories(${y_SOURCE_DIR})

set(ULTIMATE_WARNINGS "-Wdisabled-optimization -Wlogical-op -Wwrite-strings -Wmissing-include-dirs -Wnoexcept -Wredundant-decls -Wsign-promo -Wswitch-default -Wundef -Winline")
set(EXTRA_WARNINGS "-pedantic-errors -Woverloaded-virtual -Wcast-qual -Wcast-align -Wunreachable-code -Wold-style-cast -Wuseless-cast -Wnon-virtual-dtor")

add_definitions("${EXTRA_WARNINGS}")
add_definitions("${ULTIMATE_WARNINGS}")
add_definitions("-std=c++1z -pedantic -Wall -Wextra")


file(GLOB_RECURSE SOURCE_FILES
		"y/*.h"
		"y/*.cpp"
		)

add_library(y STATIC ${SOURCE_FILES})

add_executable(main ${SOURCE_FILES} "main.cpp")
target_link_libraries(main y)

target_compile_definitions(main PRIVATE -DY_AUTO_TEST)

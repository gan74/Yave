cmake_minimum_required(VERSION 3.24)
project(AngelScript)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE AS_FILES
    "angelscript/*.h"
    "angelscript/*.cpp"
    "add_on/*.h"
    "add_on/*.cpp"
)


add_library(AngelScript STATIC ${AS_FILES})
target_include_directories(AngelScript PRIVATE angelscript/include)

# https://www.angelcode.com/angelscript/sdk/docs/manual/doc_compile_lib.html
if(MSVC)
    # ml64.exe /c /nologo /Fo$(OutDir)\as_callfunc_x64_msvc_asm.obj /W3 /Zi /Ta $(InputDir)\$(InputFileName)
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/as_callfunc_x64_msvc_asm.obj"
        COMMAND ml64 /c /nologo /Fo "${CMAKE_CURRENT_BINARY_DIR}/as_callfunc_x64_msvc_asm.obj" /W3 /Zi /Ta "${CMAKE_CURRENT_SOURCE_DIR}/angelscript/source/as_callfunc_x64_msvc_asm.asm"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/angelscript/source/as_callfunc_x64_msvc_asm.asm"
        COMMENT "Assembling as_callfunc_x64_msvc_asm.asm"
    )

    add_custom_target(AngelScriptAsm DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/as_callfunc_x64_msvc_asm.obj")
    add_dependencies(AngelScript AngelScriptAsm)
    target_link_libraries(AngelScript PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/as_callfunc_x64_msvc_asm.obj")
else()
    target_compile_options(AngelScript PRIVATE "-fno-strict-aliasing")
endif()


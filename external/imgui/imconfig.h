//-----------------------------------------------------------------------------
// COMPILE-TIME OPTIONS FOR DEAR IMGUI
// Most options (memory allocation, clipboard callbacks, etc.) can be set at runtime via the ImGuiIO structure - ImGui::GetIO().
//-----------------------------------------------------------------------------
// A) You may edit imconfig.h (and not overwrite it when updating imgui, or maintain a patch/branch with your modifications to imconfig.h)
// B) or add configuration directives in your own file and compile with #define IMGUI_USER_CONFIG "myfilename.h"
// Note that options such as IMGUI_API, IM_VEC2_CLASS_EXTRA or ImDrawIdx needs to be defined consistently everywhere you include imgui.h, not only for the imgui*.cpp compilation units.
//-----------------------------------------------------------------------------

#pragma once

#include <y/math/Vec.h>
#include "IconsFontAwesome5.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wsign-promo"
#pragma GCC diagnostic ignored "-Wswitch-default"
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif

//---- Define assertion handler. Defaults to calling assert().
#define IM_ASSERT(_EXPR) do { if(!(_EXPR)) { y_fatal("IM_ASSERT failed: " #_EXPR); } } while(false)


//---- Define attributes of all API symbols declarations, e.g. for DLL under Windows.
//#define IMGUI_API __declspec( dllexport )
//#define IMGUI_API __declspec( dllimport )

//---- Don't define obsolete functions names. Consider enabling from time to time or when updating to reduce likelihood of using already obsolete function/names
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

//---- Don't implement default handlers for Windows (so as not to link with certain functions)
//#define IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS   // Don't use and link with OpenClipboard/GetClipboardData/CloseClipboard etc.
//#define IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS         // Don't use and link with ImmGetContext/ImmSetCompositionWindow.

//---- Don't implement demo windows functionality (ShowDemoWindow()/ShowStyleEditor()/ShowUserGuide() methods will be empty)
//---- It is very strongly recommended to NOT disable the demo windows. Please read the comment at the top of imgui_demo.cpp.
//#define IMGUI_DISABLE_DEMO_WINDOWS

//---- Don't implement ImFormatString(), ImFormatStringV() so you can reimplement them yourself.
//#define IMGUI_DISABLE_FORMAT_STRING_FUNCTIONS

//---- Include imgui_user.inl at the end of imgui.cpp so you can include code that extends ImGui using its private data/functions.
#define IMGUI_INCLUDE_IMGUI_USER_INL

//---- Include imgui_user.h at the end of imgui.h as a convenience
#define IMGUI_INCLUDE_IMGUI_USER_H

//---- Pack colors to BGRA8 instead of RGBA8 (if you needed to convert from one to another anyway)
//#define IMGUI_USE_BGRA_PACKED_COLOR

//---- Implement STB libraries in a namespace to avoid linkage conflicts (defaults to global namespace)
#define IMGUI_STB_NAMESPACE     ImGuiStb

//---- Define constructor and implicit cast operators to convert back<>forth from your math types and ImVec2/ImVec4.
// This will be inlined as part of ImVec2 and ImVec4 class declarations.

#define IM_VEC2_CLASS_EXTRA                                                                     \
        ImVec2(const y::math::Vec2& f) { x = f.x(); y = f.y(); }                                \
        operator y::math::Vec2() const { return y::math::Vec2(x, y); }                          \
        ImVec2(const y::math::Vec2i& f) { x = float(f.x()); y = float(f.y()); }                 \
        operator y::math::Vec2i() const { return y::math::Vec2i(y::i32(x), y::i32(y)); }        \
        ImVec2(const y::math::Vec2ui& f) { x = float(f.x()); y = float(f.y()); }                \
        operator y::math::Vec2ui() const {                                                      \
            y_debug_assert(x >= 0.0f && y >= 0.0f);                                             \
            return y::math::Vec2ui(y::u32(x), y::u32(y));                                       \
        }

#define IM_VEC4_CLASS_EXTRA                                                                     \
        ImVec4(const y::math::Vec4& f) { x = f.x(); y = f.y(); z = f.z(); w = f.w(); }          \
        operator y::math::Vec4() const { return  y::math::Vec4(x, y, z, w); }


//---- Use 32-bit vertex indices (instead of default 16-bit) to allow meshes with more than 64K vertices. Render function needs to support it.
#define ImDrawIdx y::u32

#define IM_DEBUG_BREAK() y_breakpoint

//#define ImTextureID const void*


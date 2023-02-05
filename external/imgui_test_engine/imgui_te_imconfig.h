// dear imgui test engine
// (template for compile-time configuration)
// Replicate or #include this file in your imconfig.h to enable test engine.



// Compile Dear ImGui with test engine hooks
// (Important: This is a value-less define, to be consistent with other defines used in core dear imgui.)
#define IMGUI_ENABLE_TEST_ENGINE

// [Optional, default 0] Enable plotting of perflog data for comparing performance of different runs.
// This feature requires ImPlot to be linked in the application.
#ifndef IMGUI_TEST_ENGINE_ENABLE_IMPLOT
#define IMGUI_TEST_ENGINE_ENABLE_IMPLOT 0
#endif

// [Optional, default 1] Enable screen capture and PNG/GIF saving functionalities
// There's not much point to disable this but we provide it to reassure user that the dependencies on imstb_image_write.h and ffmpeg are technically optional.
#ifndef IMGUI_TEST_ENGINE_ENABLE_CAPTURE
#define IMGUI_TEST_ENGINE_ENABLE_CAPTURE 1
#endif

// [Optional, default 0] Using std::function and <functional> for function pointers such as ImGuiTest::TestFunc and ImGuiTest::GuiFunc
#ifndef IMGUI_TEST_ENGINE_ENABLE_STD_FUNCTION
#define IMGUI_TEST_ENGINE_ENABLE_STD_FUNCTION 0
#endif

// [Optional, default 0] Automatically fill ImGuiTestEngineIO::CoroutineFuncs with a default implementation using std::thread
#ifndef IMGUI_TEST_ENGINE_ENABLE_COROUTINE_STDTHREAD_IMPL
#define IMGUI_TEST_ENGINE_ENABLE_COROUTINE_STDTHREAD_IMPL 0
#endif


// Test Engine Assert Macro
// - Macro is calling IM_DEBUG_BREAK() inline to get a callstack in the caller function.
// - Macro is using comma operator instead of an if() to avoid "conditional expression is constant" warnings.
extern void ImGuiTestEngine_Assert(const char* expr, const char* file, const char* func, int line);
#define IM_TEST_ENGINE_ASSERT(_EXPR)    do { if ((void)0, !(_EXPR)) { ImGuiTestEngine_Assert(#_EXPR, __FILE__, __func__, __LINE__); IM_DEBUG_BREAK(); } } while (0)


// Bind Main Assert macro

#if 0
#ifdef IM_ASSERT
#undef IM_ASSERT
#endif

extern bool imgui_test_engine_running();
#define IM_ASSERT(_EXPR) do {                   \
    if(imgui_test_engine_running()) {           \
        IM_TEST_ENGINE_ASSERT(_EXPR);           \
    } else {                                    \
        IM_Y_ASSERT(_EXPR);                     \
    }                                           \
} while(false)
#endif

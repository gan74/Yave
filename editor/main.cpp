/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************/

#include "ImGuiPlatform.h"
#include "EditorApplication.h"

#include <editor/utils/crashhandler.h>

#include <yave/graphics/device/Instance.h>
#include <y/concurrent/concurrent.h>

#include <y/math/random.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

#ifdef Y_OS_WIN
#include <windows.h>
#endif

using namespace editor;

static bool debug_instance = is_debug_defined;
static bool multi_viewport = true;
static bool run_tests = false;


static void parse_args(int argc, char** argv) {
    for(std::string_view arg : core::Span<const char*>(argv + 1, argc - 1)) {
        if(arg == "--nodebug") {
            debug_instance = false;
        } else if(arg == "--debug") {
            debug_instance = true;
        } else if(arg == "--nomv") {
            multi_viewport = false;
        } else if(arg == "--run-tests") {
            run_tests = true;
        } else if(arg == "--errbreak") {
#ifdef Y_DEBUG
            core::result::break_on_error = true;
#else
            log_msg(fmt("{} is not supported unless Y_DEBUG is defined", arg), Log::Error);
#endif
        } else if(arg == "--waitdbg") {
#ifdef Y_OS_WIN
            while(!::IsDebuggerPresent()) {
                core::Duration::sleep(core::Duration::milliseconds(10));
            }
#else
            log_msg(fmt("{} is not supported outside of Windows", arg), Log::Error);
#endif
        } else {
            log_msg(fmt("Unknown argumeent: {}", arg), Log::Error);
        }
    }

    y_debug_assert([] { log_msg("Debug asserts enabled."); return true; }());
}

static Instance create_instance() {
    y_profile();
    if(!debug_instance) {
        log_msg("Vulkan debugging disabled.", Log::Warning);
    }
    return Instance(debug_instance ? DebugParams::debug() : DebugParams::none());
}




#include <yave/world/World.h>


void test_arch() {
    World world;

    core::Vector<std::pair<usize, UntypedNodeRef>> refs;

    const usize count = 20000;
    for(usize i = 0; i != count; ++i) {
        refs.emplace_back(i, world.add<usize>(i));
    }

    math::FastRandom rng;
    for(usize i = 0; i != count / 4; ++i) {
        const usize index = usize(rng()) % refs.size();
        const auto ref = refs[index].second;
        refs.erase_unordered(refs.begin() + index);
        const auto typed = ref.to_typed<usize>();

        y_debug_assert(typed.get());
        world.remove(typed);
        y_debug_assert(!typed.get());
    }

    for(usize i = 0; i != refs.size(); ++i) {
        const auto& [index, ref] = refs[i];
        y_debug_assert(ref.is<usize>());
        const auto typed = ref.to_typed<usize>();
        y_debug_assert(*typed.get() == index);
    }

    log_msg(fmt("page size for usize: {}", yave::detail::Page<usize>::element_count));
    log_msg("Ok");
}



int main(int argc, char** argv) {
    test_arch();

    return 0;


    concurrent::set_thread_name("Main thread");

    parse_args(argc, argv);

    if(!crashhandler::setup_handler()) {
        log_msg("Unable to setup crash handler.", Log::Warning);
    }

    Instance instance = create_instance();

    init_device(instance);
    y_defer(destroy_device());


    {
        ImGuiPlatform platform(multi_viewport, run_tests);
        EditorApplication editor(&platform);
        editor.exec();
    }

    log_msg("exiting...");

    return 0;
}


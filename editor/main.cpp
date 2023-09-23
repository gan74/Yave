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

    y_debug_assert(component_type<int>() == component_type<const int&>());

    {
        const auto id = world.create_entity();
        world.add<u32>(id, 1);
        world.add<int>(id, 1);
    }

    {
        const auto id = world.create_entity();
        world.add<u32>(id, 2);
    }

    {
        const auto id = world.create_entity();
        world.add<int>(id, 3);
        world.add<u32>(id, 3);
        world.remove<u32>(id);
    }

    {
        const auto id = world.create_entity();
        world.add<u32>(id, 4);
        world.add<int>(id, 4);
        world.add<u64>(id, 4);
    }

    {
        const auto id = world.create_entity();
        auto ref = world.add<i16>(id);
        *ref.get() = 1;
        *ref.get() = 5;
    }

    {
        const auto id = world.create_entity();
        world.add<u32>(id, 6);
        const auto ref = world.add<int>(id, 6);
        world.remove_entity(id);

        y_debug_assert(*ref.get() == 6);
        world.flush();
        y_debug_assert(!ref.get());
    }



    {
        log_msg("Simple");
        const auto q = world.query<int, u32>();
        y_debug_assert(q.size() == 2);
        y_debug_assert(*q.get<int>(0) == 1);
        y_debug_assert(*q.get<int>(1) == 4);
        log_msg("  ok");
    }

    {
        log_msg("Mut");
        const auto q = world.query<i16>();
        y_debug_assert(q.size() == 1);
        y_debug_assert(*q.get<i16>(0) == 5);
        log_msg("  ok");
    }

    {
        log_msg("Not");
        const auto q = world.query<Not<u64>, Mut<u32>>();
        y_debug_assert(q.size() == 2);
        y_debug_assert(*q.get<u32>(0) == 1);
        y_debug_assert(*q.get<u32>(1) == 2);
        log_msg("  ok");
    }

    {
        log_msg("Not driver");
        const auto q = world.query<Not<int>, u32>();
        y_debug_assert(q.size() == 1);
        log_msg("  ok");
    }
}

void bench_arch() {
    y_profile();

    World world;

    core::Vector<EntityId> ids;
    for(usize i = 0; i != 10000; ++i) {
        const auto id = world.create_entity();

        if(i % 2 == 0) {
            world.add<usize>(id, i);
        }
        if(i % 3 == 0) {
            world.add<int>(id, int(i));
        }
        if(i % 5 == 0) {
            world.add<u32>(id, u32(i));
        }

        ids << id;
    }

    for(const EntityId id : ids) {
        world.remove_entity(id);
    }

    core::DebugTimer _("flush");
    world.flush();
}


int main(int argc, char** argv) {
    test_arch();
    bench_arch();

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


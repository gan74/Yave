/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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

#include "editor.h"
#include "ImGuiPlatform.h"

#include <editor/utils/crashhandler.h>

#include <yave/graphics/device/Instance.h>
#include <yave/ecs/EntityWorld.h>
#include <yave/ecs/traits.h>

#include <y/concurrent/concurrent.h>
#include <y/concurrent/Signal.h>

#include <y/utils/log.h>
#include <y/utils/format.h>
#include <y/concurrent/JobSystem.h>

#ifdef Y_OS_WIN
#include <windows.h>
#endif

using namespace editor;

static bool multi_viewport = false;
static InstanceParams inst_params = {
    .validation_layers = is_debug_defined,
    .debug_utils = is_debug_defined,
};


static void parse_args(int argc, char** argv) {
    for(std::string_view arg : core::Span<const char*>(argv + 1, argc - 1)) {
        if(arg == "--nodebug") {
            inst_params.validation_layers = false;
        } else if(arg == "--debug") {
            inst_params.validation_layers = true;
        } else if(arg == "--nort") {
            inst_params.raytracing = false;
        } else if(arg == "--nomv") {
            multi_viewport = false;
        } else if(arg == "--mv") {
            multi_viewport = true;
        } else if(arg == "--errbreak") {
#ifdef Y_DEBUG
            core::result::break_on_error = true;
#else
            log_msg(fmt("{} is not supported unless Y_DEBUG is defined", arg), Log::Error);
#endif
        } else if(arg == "--waitdbg") {
#ifdef Y_OS_WIN
            if(!::IsDebuggerPresent()) {
                ::MessageBoxA(nullptr, "Attach debugger now", "Attach debugger now", MB_OK);
            }
#else
            log_msg(fmt("{} is not supported outside of Windows", arg), Log::Error);
#endif
        } else {
            log_msg(fmt("Unknown command line argument: {}", arg), Log::Error);
        }
    }

    y_debug_assert([] { log_msg("Debug asserts enabled"); return true; }());
}

static Instance create_instance() {
    if(!inst_params.validation_layers) {
        log_msg("Vulkan validation disabled", Log::Warning);
    }
    return Instance(inst_params);
}



void sleep() {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}





using JobHandle = concurrent::JobSystem::JobHandle;

struct JobInOut {
    core::Vector<ecs::ComponentTypeIndex> writing;
    core::Vector<ecs::ComponentTypeIndex> reading;

    template<typename T>
    void fill_one() {
        const ecs::ComponentTypeIndex t = ecs::type_index<ecs::traits::component_raw_type_t<T>>();
        if constexpr(ecs::traits::is_component_mutable<T>) {
            writing << t;
        } else {
            reading << t;
        }
    }

    template<typename... Ts>
    void fill_from_group(ecs::EntityGroup<Ts...>*) {
        (fill_one<Ts>(), ...);
    }

    template<typename T>
    void fill_from_group() {
        fill_from_group(static_cast<std::remove_cvref_t<T>*>(nullptr));
    }

    template<typename... Ts>
    static JobInOut make() {
        JobInOut io;
        (io.fill_one<Ts>(), ...);
        return io;
    }
};

template<typename T, typename... Args>
struct JobInOutMaker : JobInOutMaker<decltype(&T::operator())> {};

template<typename Ret, typename... Args>
struct JobInOutMaker<Ret(*)(Args...)> : JobInOutMaker<Ret(Args...)> {};

template<typename Ret, typename... Args>
struct JobInOutMaker<Ret(&)(Args...)> : JobInOutMaker<Ret(Args...)> {};

template<typename T, typename Ret, typename... Args>
struct JobInOutMaker<Ret(T::*)(Args...)> : JobInOutMaker<Ret(Args...)> {};

template<typename T, typename Ret, typename... Args>
struct JobInOutMaker<Ret(T::*)(Args...) const> : JobInOutMaker<Ret(Args...)> {};

template<typename Ret, typename... Args>
struct JobInOutMaker<Ret(Args...)> {
    static JobInOut make() {
        JobInOut io;
        (io.fill_from_group<Args>(), ...);
        return io;
    }
};


class EntityGroupResolver {
    public:
        EntityGroupResolver() = default;
        EntityGroupResolver(ecs::EntityWorld& world) : _world(&world) {
        }

        template<typename... Ts>
        operator ecs::EntityGroup<Ts...>() const {
            return _world->create_group<Ts...>();
        }

    private:
        ecs::EntityWorld* _world = nullptr;
};

struct Sched {
    struct Handle {
        usize index;
    };

    struct Job {
        JobInOut io;
        core::Vector<Handle> dependencies;

        std::function<void(ecs::EntityWorld&)> func;
        JobHandle handle;
    };

    core::Vector<Job> _jobs;


    template<typename F>
    Handle shed(JobInOut io, F&& func, core::Span<Handle> deps = {}) {
        _jobs.emplace_back(std::move(io), core::Vector<Handle>(deps), y_fwd(func), JobHandle());
        return {_jobs.size() - 1};
    }


    template<typename F>
    Handle shed(F&& func, core::Span<Handle> deps = {}) {
        JobInOut io = JobInOutMaker<F>::make();
        return shed(std::move(io), [func](ecs::EntityWorld& world) {
            std::array<EntityGroupResolver, function_traits<F>::arg_count> args;
            for(EntityGroupResolver& arg : args) {
                arg = EntityGroupResolver(world);
            }
            std::apply(func, args);
        }, deps);
    }


    void run_sched(ecs::EntityWorld& world, concurrent::JobSystem& job_system) {
        y_profile();

        core::FlatHashMap<ecs::ComponentTypeIndex, core::Vector<JobHandle>> writing;
        core::FlatHashMap<ecs::ComponentTypeIndex, core::Vector<JobHandle>> reading;

        for(Job& job : _jobs) {
            core::Vector<JobHandle> to_wait;

            for(const Handle& dep : job.dependencies) {
                const JobHandle& h = _jobs[dep.index].handle;
                y_debug_assert(!h.is_empty());
                to_wait << h;
            }

            for(const ecs::ComponentTypeIndex& index : job.io.writing) {
                const core::Vector<JobHandle>& w = writing[index];
                to_wait.push_back(w.begin(), w.end());

                const core::Vector<JobHandle>& r = reading[index];
                to_wait.push_back(r.begin(), r.end());
            }

            for(const ecs::ComponentTypeIndex& index : job.io.reading) {
                const core::Vector<JobHandle>& w = writing[index];
                to_wait.push_back(w.begin(), w.end());
            }

            std::sort(to_wait.begin(), to_wait.end());
            const auto end = std::unique(to_wait.begin(), to_wait.end());

            job.handle = job_system.schedule([&] { job.func(world); }, core::Span(to_wait.data(), end - to_wait.begin()));

            for(const ecs::ComponentTypeIndex& index : job.io.writing) {
                writing[index] << job.handle;
            }

            for(const ecs::ComponentTypeIndex& index : job.io.reading) {
                reading[index] << job.handle;
            }
        }

        {
            y_profile_zone("wait");
            for(Job& job : _jobs) {
                job.handle.wait();
            }
        }
    }
};


int main(int argc, char** argv) {
    concurrent::set_thread_name("Main thread");

    parse_args(argc, argv);

    if(!crashhandler::setup_handler()) {
        log_msg("Unable to setup crash handler", Log::Warning);
    }


    ::MessageBoxA(nullptr, "ready", "ready", 0);

    Sched s;

#define REPEAT(x) for(usize _i = 0; _i != (x); ++_i)

    Sched::Handle wait;

    REPEAT(1) {
        wait = s.shed([](ecs::EntityGroup<ecs::Mutate<int>, ecs::Mutate<float>>&&) {
            y_profile_zone("write: [int, float]");
            sleep();
        });
    }

    REPEAT(2) {
        s.shed([](ecs::EntityGroup<int>&&) {
            y_profile_zone("read: [int]");
            sleep();
        });
    }

    REPEAT(2) {
        s.shed([](ecs::EntityGroup<float>&&) {
            y_profile_zone("read: [float]");
            sleep();
        });
    }

    REPEAT(1) {
        s.shed([](ecs::EntityGroup<ecs::Mutate<int>>&&) {
            y_profile_zone("write: [int]");
            sleep();
        });
    }

    REPEAT(2) {
        s.shed([](ecs::EntityGroup<ecs::Mutate<float>>&&) {
            y_profile_zone("write: [float]");
            sleep();
        });
    }

    REPEAT(32) {
        s.shed([](ecs::EntityGroup<double>&&) {
            y_profile_zone("read: [double]");
            sleep();
        });
    }


    ecs::EntityWorld world;

    {
        concurrent::JobSystem js(4);
        s.run_sched(world, js);
    }


    /*{
        Instance instance = create_instance();

        init_device(instance);
        y_defer(destroy_device());



        ImGuiPlatform platform(multi_viewport);
        init_editor(&platform, Settings::load());
        y_defer(destroy_editor());

        run_editor();
    }

    app_settings().save();

    log_msg("exiting...");*/

    return 0;
}


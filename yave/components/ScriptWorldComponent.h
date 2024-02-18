/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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
#ifndef YAVE_COMPONENTS_SCRIPTWORLDCOMPONENT_H
#define YAVE_COMPONENTS_SCRIPTWORLDCOMPONENT_H

#include <yave/yave.h>

#include <y/core/Vector.h>
#include <y/core/String.h>
#include <y/core/Result.h>

#include <y/reflect/reflect.h>

namespace yave {

class ScriptWorldComponent final {

    public:
        struct Script {
            core::String name;
            core::String code;

            y_reflect(Script, name, code);
        };

        struct OneShotScript {
            core::String code;
            bool done = false;

            y_reflect(OneShotScript, code);
        };


        auto& scripts() {
            return _scripts;
        }

        auto& one_shot() {
            return _one_shot;
        }


        struct RunOnceResult {
            bool done = false;
            core::Result<void, core::String> result = core::Ok();
        };

        struct RunOnce {
            core::String code;
            std::shared_ptr<RunOnceResult> result;
        };

        const std::shared_ptr<RunOnceResult>& run_once(core::String code) {
            auto& once = _once.emplace_back();
            once.code = std::move(code);
            once.result = std::make_shared<RunOnceResult>();
            return once.result;
        }


        y_reflect(ScriptWorldComponent, _scripts, _one_shot)

    private:
        friend class ScriptSystem;

        core::Vector<Script> _scripts;
        core::Vector<OneShotScript> _one_shot;
        core::Vector<RunOnce> _once;
};

}

#endif // YAVE_COMPONENTS_SCRIPTWORLDCOMPONENT_H


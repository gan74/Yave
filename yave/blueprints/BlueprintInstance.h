/*******************************
Copyright (c) 2016-2026 Grégoire Angerand

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
#ifndef YAVE_BLUEPRINTS_BLUEPRINTINSTANCE_H
#define YAVE_BLUEPRINTS_BLUEPRINTINSTANCE_H

#include "BlueprintNode.h"

#include <yave/ecs/TriggerManager.h>

#include <y/core/Result.h>

#include <functional>

namespace yave {

struct BlueprintError {
    usize node_index = 0;
    core::String error;
};

struct BlueprintInstruction {
    u32 node_index = 0;
    std::function<void()> func; // may throw std::runtime_error
};

struct BlueprintParam {
    core::String name;
    i32 order = 0;
    const BlueprintParamType* type = nullptr;
    const void* ptr = nullptr;
};

class BlueprintStorage : NonCopyable {
    public:
        static inline constexpr usize storage_chunk_size = 4 * 1024;

        BlueprintStorage() = default;

        void* alloc(usize size, usize alignment);

    private:
        core::Vector<core::FixedArray<u8>> _chunks;
        usize _offset = 0;
};

class BlueprintInstance : NonCopyable {
    public:
        struct EntryPoint {
            u32 node_index = 0;
            ecs::TriggerTypeIndex trigger_type = {};
            void* payload = nullptr;
            usize payload_size = 0;
            void (*subscribe)(ecs::TriggerSubscriber&) = nullptr;
            core::Vector<BlueprintInstruction> instructions;
        };

        BlueprintInstance() = default;

        core::Span<EntryPoint> entry_points() const;
        core::Span<BlueprintParam> params_in() const;
        core::Span<BlueprintParam> params_out() const;

        core::Result<void, BlueprintError> trigger(const EntryPoint& entry_point, const void* payload) noexcept;

    private:
        friend class Blueprint;

        BlueprintStorage _storage;
        core::Vector<EntryPoint> _entry_points;
        core::Vector<BlueprintParam> _params_in;
        core::Vector<BlueprintParam> _params_out;
};

}

#endif // YAVE_BLUEPRINTS_BLUEPRINTINSTANCE_H
